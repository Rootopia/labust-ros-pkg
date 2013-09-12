/*********************************************************************
 * Software License Agreement (BSD License)
 *
 *  Copyright (c) 2010, LABUST, UNIZG-FER
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.
 *   * Neither the name of the LABUST nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 *  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 *  COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 *  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 *  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 *  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 *  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 *
 *  Author: Dula Nad
 *  Created: 06.05.2013.
 *********************************************************************/
#include <labust/control/BenchRadio.hpp>
#include <labust/tools/conversions.hpp>

#include <boost/archive/binary_oarchive.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/bind.hpp>
#include <boost/regex.hpp>

#include <std_msgs/Bool.h>

#include <stdexcept>

using labust::control::BenchRadio;

BenchRadio::BenchRadio():
				port(io),
				isBench(true)
	{this->onInit();}

BenchRadio::~BenchRadio()
{
	io.stop();
	iorunner.join();
}

void BenchRadio::onInit()
{
	ros::NodeHandle nh,ph("~");

	std::string portName("/dev/ttyUSB0");
	int baud(9600);
	ph.param("PortName",portName,portName);
	ph.param("BaudRate",baud,baud);
	ph.param("IsBench",isBench,isBench);

	port.open(portName);
	port.set_option(boost::asio::serial_port::baud_rate(baud));

	if (!port.is_open())
	{
		ROS_ERROR("Cannot open port.");
		throw std::runtime_error("Unable to open the port.");
	}

	if (isBench)
	{
		tauIn = nh.subscribe<auv_msgs::BodyForceReq>("tauIn",1,&BenchRadio::onTauIn,this);

		tauAch = nh.advertise<auv_msgs::BodyForceReq>("tauAch",1);
		imuOut = nh.advertise<sensor_msgs::Imu>("imu",1);
		gpsOut = nh.advertise<sensor_msgs::NavSatFix>("gps",1);
	}
	else
	{
		tauOut = nh.advertise<auv_msgs::BodyForceReq>("tauOut",1);

		imuIn = nh.subscribe<sensor_msgs::Imu>("imu",1,&BenchRadio::onImu,this);
		tauAchIn = nh.subscribe<auv_msgs::BodyForceReq>("tauAch",1,&BenchRadio::onTauAchIn,this);
		gpsIn = nh.subscribe<sensor_msgs::NavSatFix>("gps",1,&BenchRadio::onGps,this);
	}

	this->start_receive();
	iorunner = boost::thread(boost::bind(&boost::asio::io_service::run,&io));
}

void BenchRadio::onTauIn(const auv_msgs::BodyForceReq::ConstPtr& tauIn)
{
	assert(isBench && "Only the radio on the bench/topside should receive tau.");
	data.surgeForce = tauIn->wrench.force.x;
	data.torqueForce = tauIn->wrench.torque.z;

	boost::asio::streambuf output;
	std::ostream out(&output);
	//Prepare sync header
	out<<"@ONTOP";
	boost::archive::binary_oarchive dataSer(output, boost::archive::no_header);
	dataSer << data;

	//write data
	int n = boost::asio::write(port, output.data());
	ROS_INFO("Bench transferred:%d",n);
}

void BenchRadio::onImu(const sensor_msgs::Imu::ConstPtr& imu)
{
	boost::mutex::scoped_lock l(cdataMux);
	double roll, pitch, yaw;
	Eigen::Quaternion<double> q(imu->orientation.x,
			imu->orientation.y, imu->orientation.z, imu->orientation.w);
	labust::tools::eulerZYXFromQuaternion(q, roll, pitch,yaw);
	cdata.yaw = yaw;
}

void BenchRadio::onGps(const sensor_msgs::NavSatFix::ConstPtr& gps)
{
	boost::mutex::scoped_lock l(cdataMux);
	cdata.lat = gps->latitude;
	cdata.lon = gps->longitude;
}

void BenchRadio::onTauAchIn(const auv_msgs::BodyForceReq::ConstPtr& tau)
{
	boost::mutex::scoped_lock l(cdataMux);
	cdata.surgeAch = tau->wrench.force.x;
	cdata.torqueAch = tau->wrench.torque.z;
	cdata.windupS = tau->disable_axis.x;
	cdata.windupT = tau->disable_axis.yaw;
}

void BenchRadio::start_receive()
{
	boost::asio::async_read(port, sbuffer.prepare(sync_length),
			boost::bind(&BenchRadio::onSync,this,_1,_2));
}

void BenchRadio::onSync(const boost::system::error_code& error, const size_t& transferred)
{
	if (!error)
	{
		sbuffer.commit(transferred);

		if (transferred == 1)
		{
			ringBuffer.push_back(sbuffer.sbumpc());
      if (ringBuffer.size() > sync_length)
      {
         ringBuffer.erase(ringBuffer.begin());
      }
		}
		else
		{
			std::istream is(&sbuffer);
			ringBuffer.clear();
			is >> ringBuffer;
		}

		if ((ringBuffer.size() >= sync_length) && (ringBuffer.substr(0,sync_length) == "@ONTOP"))
		{
			ROS_INFO("Synced on @ONTOP");
			boost::asio::async_read(port,sbuffer.prepare(Bench_package_length),boost::bind(&BenchRadio::onIncomingData,this,_1,_2));
		}
		else if ((ringBuffer.size() >= sync_length) && (ringBuffer.substr(0,sync_length) == "@CART2"))
		{
			ROS_INFO("Synced on @CART2");
			boost::asio::async_read(port,sbuffer.prepare(cart_package_length),boost::bind(&BenchRadio::onIncomingData,this,_1,_2));
		}
		else
		{
			ROS_INFO("No sync: %s",ringBuffer.c_str());
			boost::asio::async_read(port, sbuffer.prepare(1),
					boost::bind(&BenchRadio::onSync,this,_1,_2));
		}
	}
}

void BenchRadio::onIncomingData(const boost::system::error_code& error, const size_t& transferred)
{
	sbuffer.commit(transferred);
	boost::archive::binary_iarchive dataSer(sbuffer, boost::archive::no_header);

	ROS_INFO("Received data: %d",transferred);

	if (!isBench)
	{
			dataSer >> data;

			boost::asio::streambuf output;
			std::ostream out(&output);
			//Prepare sync header
			out<<"@CART2";
			boost::archive::binary_oarchive dataSer(output, boost::archive::no_header);
			boost::mutex::scoped_lock l(cdataMux);
			//cdata.time = ros::Time::now().toSec();
			dataSer << cdata;
			l.unlock();
			//write data
			int n=boost::asio::write(port, output.data());
			ROS_INFO("Cart transferred:%d",n);

			auv_msgs::BodyForceReq tau;
			tau.wrench.force.x = data.surgeForce;
			tau.wrench.torque.z = data.torqueForce;
			tauOut.publish(tau);
	}
	else
	{
		dataSer >> cdata;

		//ROS_INFO("Time diff:%f",ros::Time::now().toSec() - cdata.time);

		auv_msgs::BodyForceReq tau;
		tau.wrench.force.x = cdata.surgeAch;
		tau.wrench.torque.z = cdata.torqueAch;
		tau.disable_axis.x = cdata.windupS;
		tau.disable_axis.yaw = cdata.windupT;
		tauAch.publish(tau);

		sensor_msgs::Imu imu;
		imu.header.stamp = ros::Time::now();
		Eigen::Quaternion<float> q;
		labust::tools::quaternionFromEulerZYX(0,0,cdata.yaw,q);
		imu.orientation.x = q.x();
		imu.orientation.y = q.y();
		imu.orientation.z = q.z();
		imu.orientation.w = q.w();
		imuOut.publish(imu);

		sensor_msgs::NavSatFix gps;
		gps.header.stamp = ros::Time::now();
		gps.latitude = cdata.lat;
		gps.longitude = cdata.lon;
		gpsOut.publish(gps);
	}

	start_receive();
}

void BenchRadio::start()
{
	ros::spin();
}

