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
 *  Author : Dula Nad
 *  Created: 23.01.2013.
 *********************************************************************/
#include <labust/navigation/NavQuestNode.hpp>
#include <labust/navigation/DVLdataClass.h>
#include <labust/navigation/NavQuestMessages.hpp>
#include <labust/archive/delimited_iarchive.hpp>
#include <labust/archive/delimited_oarchive.hpp>
#include <labust/preprocessor/clean_serializator.hpp>
#include <labust/tools/conversions.hpp>

#include <geometry_msgs/TwistStamped.h>
#include <std_msgs/Bool.h>

#include <boost/bind.hpp>
#include <boost/regex.hpp>
#include <boost/serialization/string.hpp>

#include <iosfwd>

PP_LABUST_CLEAN_ARRAY_OSERIALIZATOR_IMPL(labust::archive::delimited_oarchive)
PP_LABUST_CLEAN_ARRAY_ISERIALIZATOR_IMPL(labust::archive::delimited_iarchive)

using namespace labust::navigation;

int labust::navigation::error_code(const NQRes& data)
{
	std::stringstream ss(data.error_code);
	int error;
	ss>>std::hex>>error;
	return error;
}

NavQuestNode::NavQuestNode():
					io(),
					port(io)
{
	this->onInit();
}

NavQuestNode::~NavQuestNode()
{
	io.stop();
	runner.join();
}

void NavQuestNode::onInit()
{
	ros::NodeHandle nh;
	ros::Rate r(1);
	bool setupOk(false);
	while (!(setupOk = this->setup_port()) && ros::ok())
	{
		ROS_ERROR("NavQuestNode::Failed to open port.");
		r.sleep();
	}

	if (setupOk)
	{
		//Advertise beam data
		beam_pub["velo_rad"].reset(new NavQuestBP(nh, "velo_rad"));
		beam_pub["wvelo_rad"].reset(new NavQuestBP(nh, "wvelo_rad"));
		speed_pub["velo_instrument"].reset(
				new TwistPublisher(nh, "velo_instrument", "base_link"));
		speed_pub["velo_earth"].reset(
				new TwistPublisher(nh, "velo_earth", "local"));
		speed_pub["water_velo_instrument"].reset(
				new TwistPublisher(nh, "water_velo_instrument", "base_link"));
		speed_pub["water_velo_earth"].reset(
				new TwistPublisher(nh, "water_velo_earth", "local"));
		lock = nh.advertise<std_msgs::Bool>("dvl_bottom",1);

		//Start the receive cycle
		this->start_receive();
		runner = boost::thread(boost::bind(&boost::asio::io_service::run,&io));
		setup_messaging();
	}
}

void NavQuestNode::setup_messaging()
{
	//Setup map for more than one message
}

void NavQuestNode::setup_publishers()
{
	//Setup maps for publishing
}

void NavQuestNode::start_receive()
{
	using namespace boost::asio;
	async_read_until(port, buffer,
			boost::regex("\r\n"),
			boost::bind(&NavQuestNode::onDvlData, this, _1,_2));
}

bool NavQuestNode::setup_port()
{
	ros::NodeHandle ph("~");
	std::string portName("/dev/ttyUSB0");
	int baud(115200);

	ph.param("PortName",portName,portName);
	ph.param("Baud",baud,baud);

	using namespace boost::asio;
	port.open(portName);
	port.set_option(serial_port::baud_rate(baud));
	port.set_option(serial_port::flow_control(
			serial_port::flow_control::none));

	return port.is_open();
}

bool NavQuestNode::test_header(const std::string& match, const std::string& stream)
{
	return stream.substr(0,match.size()) == match;
}

void NavQuestNode::publishDvlData(const NQRes& data)
{
	geometry_msgs::TwistStamped::Ptr twist(new geometry_msgs::TwistStamped());

	(*beam_pub["velo_rad"])(data.velo_rad);
	(*beam_pub["wvelo_rad"])(data.wvelo_rad);
	(*speed_pub["velo_instrument"])(data.velo_instrument);
	(*speed_pub["velo_earth"])(data.velo_earth);
	(*speed_pub["water_velo_instrument"])(data.water_velo_instrument);
	(*speed_pub["water_velo_earth"])(data.water_velo_earth);

	enum{valid_flag=3};
	bool water_lock= (data.velo_instrument[valid_flag]==2) || (data.velo_earth[valid_flag]==2);
	bool valid=data.velo_instrument[valid_flag] && data.velo_earth[valid_flag];
	std_msgs::Bool bottom_lock;
	bottom_lock.data = !water_lock && valid;
	lock.publish(bottom_lock);
}

void NavQuestNode::conditionDvlData(const NQRes& data)
{}

void NavQuestNode::onDvlData(const boost::system::error_code& e,
		std::size_t size)
{
	if (!e)
	{
		std::istream is(&buffer);
		std::string data(size,'\0');
		is.read(&data[0],size);

		if (test_header("$#NQ.RES", data))
		{
			NQRes dvl_data;
			std::istringstream is;
			is.rdbuf()->pubsetbuf(&data[0],size);
			labust::archive::delimited_iarchive ia(is);
			ia>>dvl_data;

			if (error_code(dvl_data) == 0) publishDvlData(dvl_data);

			ROS_INFO("DVL decoded: header:%s, error:%s.",
					dvl_data.header.c_str(),
					dvl_data.error_code.c_str());
		}
	}
	else
	{
		ROS_ERROR("NavQuestNode: %s",e.message().c_str());
	}
	this->start_receive();
}


int main(int argc, char* argv[])
{
	ros::init(argc,argv,"navquest_node");
	NavQuestNode node;
	ros::spin();

	return 0;
}


