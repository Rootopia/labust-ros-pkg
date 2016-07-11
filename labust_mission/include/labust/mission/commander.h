/*
 * commander.h
 *
 *  Created on: Apr 21, 2016
 *      Author: filip
 */

#ifndef LABUST_ROS_PKG_LABUST_MISSION_INCLUDE_LABUST_MISSION_COMMANDER_H_
#define LABUST_ROS_PKG_LABUST_MISSION_INCLUDE_LABUST_MISSION_COMMANDER_H_

#include <ros/ros.h>
#include <labust_mission/maneuverGenerator.hpp>
#include <boost/lexical_cast.hpp>

#include <misc_msgs/Go2depthService.h>
#include <misc_msgs/Go2pointService.h>
#include <misc_msgs/PointerService.h>
#include <misc_msgs/DPService.h>
#include <misc_msgs/LawnmoverService.h>
#include <misc_msgs/StartParser.h>

#include <std_srvs/Trigger.h>


/*********************************************************************
 *** Commander class definition
 ********************************************************************/

namespace labust
{
	namespace mission
	{
		class Commander
		{
		public:

			/*****************************************************************
			 ***  Class functions
			 ****************************************************************/

			Commander(std::string xml_path);

			~Commander();

			bool go2pointService(misc_msgs::Go2pointService::Request &req, misc_msgs::Go2pointService::Response &res);

			bool go2depthService(misc_msgs::Go2depthService::Request &req, misc_msgs::Go2depthService::Response &res);

			bool pointerService(misc_msgs::PointerService::Request &req, misc_msgs::PointerService::Response &res);

			bool dynamicPositioningService(misc_msgs::DPService::Request &req, misc_msgs::DPService::Response &res);

			bool lawnmoverService(misc_msgs::LawnmoverService::Request &req, misc_msgs::LawnmoverService::Response &res);

			bool stopService(std_srvs::Trigger::Request &req, std_srvs::Trigger::Response &res);

			bool pauseService(std_srvs::Trigger::Request &req, std_srvs::Trigger::Response &res);

			bool continueService(std_srvs::Trigger::Request &req, std_srvs::Trigger::Response &res);

			/*****************************************************************
			 ***  Helper functions
			 ****************************************************************/

			void publishEventString(std::string event);

			void publishStartParser(std::string xml);

			void saveAndRequestAction(std::string name);

			/*********************************************************************
			 ***  Class variables
			 ********************************************************************/

		private:

			/*** Subscribers ***/

			/*** Publishers ***/
			ros::Publisher pubStatus;
			ros::Publisher pubEventString, pubStartParser;

			/*** Services ***/
			ros::ServiceServer srvDepth;
			ros::ServiceServer srvGo2point;
			ros::ServiceServer srvPointer;
			ros::ServiceServer srvDynPos;
			ros::ServiceServer srvLawnomver;
			ros::ServiceServer srvStop;
			ros::ServiceServer srvPause;
			ros::ServiceServer srvContinue;


			ros::ServiceServer srvStatus; // To bi trebao mission exec publishati.

			/*** Maneuver generator class ***/
			labust::maneuver::ManeuverGenerator MG;

			/*** Path for saving mission xml files ***/
			std::string xml_save_path;

		};

		Commander::Commander(std::string xml_path):
				MG(xml_path),
				xml_save_path("")
		{
			ros::NodeHandle nh;

			nh.param("mission_save_path",xml_save_path,xml_save_path);

			/*** Subscribers ***/
			//subStateHatAbs = nh.subscribe<auv_msgs::NavSts>("stateHatAbs",1,&DataEventManager::onStateHat,this);

			/*** Publishers ***/
			pubEventString = nh.advertise<std_msgs::String>("eventString",1);
			pubStartParser = nh.advertise<misc_msgs::StartParser>("startParser",1);

			/*** Services ***/
			srvDepth = nh.advertiseService("commander/go2depth", &Commander::go2depthService,this);
			srvGo2point = nh.advertiseService("commander/go2point", &Commander::go2pointService,this);
			srvPointer = nh.advertiseService("commander/pointer", &Commander::pointerService,this);
			srvLawnomver = nh.advertiseService("commander/lawnmover", &Commander::lawnmoverService,this);
			srvStop = nh.advertiseService("commander/stop_mission", &Commander::stopService,this);
			srvPause = nh.advertiseService("commander/pause_mission", &Commander::pauseService,this);
			srvContinue = nh.advertiseService("commander/continue_mission", &Commander::continueService,this);
		}

		Commander::~Commander()
		{

		}

		/*** go2depth service ***/
		bool Commander::go2depthService(misc_msgs::Go2depthService::Request &req, misc_msgs::Go2depthService::Response &res)
		{
			/*** Generate mission xml file ***/
			MG.writeXML.addMission();
			MG.generateGo2Point(true,0,0,req.depth,0,0.2,0.1,false,false,true,false,false,"","");
			/*** Request mission execution ***/
			saveAndRequestAction("go2depth");

			res.status = true;
			return true;
		}

		/*** go2point service ***/
		bool Commander::go2pointService(misc_msgs::Go2pointService::Request &req, misc_msgs::Go2pointService::Response &res)
		{
			/*** Generate mission xml file ***/
			MG.writeXML.addMission();
			MG.generateGo2Point_FA(req.point.x, req.point.y, req.point.z, req.speed, 1.5);

			/*** Request mission execution ***/
			saveAndRequestAction("go2point");

			res.status = true;
			return true;
		}

		/*** Pointer service ***/
		bool Commander::pointerService(misc_msgs::PointerService::Request &req, misc_msgs::PointerService::Response &res)
		{
			/*** Generate mission xml file ***/
			MG.writeXML.addMission();
			MG.generatePointer(req.radius,
					req.vertical_offset,
					req.guidance_target.x,
					req.guidance_target.y,
					req.guidance_target.z,
					req.guidance_enable,
					req.wrapping_enable,
					req.streamline_orientation,
					req.guidance_topic,
					req.radius_topic);

			/*** Request mission execution ***/
			saveAndRequestAction("pointer");

			res.status = true;
			return true;
		}

        /*** Pointer service ***/
        bool Commander::dynamicPositioningService(misc_msgs::DPService::Request &req, misc_msgs::DPService::Response &res)
        {
            /*** Generate mission xml file ***/
            MG.writeXML.addMission();
            MG.generateDynamicPositioning(
                            req.north,
                            req.east,
                            req.depth,
                            req.heading,
                            req.north_enable,
                            req.east_enable,
                            req.depth_enable,
                            req.heading_enable,
                            req.altitude_enable,
                            req.track_heading_enable,
                            req.point_to_target,
                            req.target_topic,
                            req.heading_topic);

            /*** Request mission execution ***/
            saveAndRequestAction("dynamic_positioning");

            res.status = true;
            return true;
        }

		/*** Lawnmover service ***/
		bool Commander::lawnmoverService(misc_msgs::LawnmoverService::Request &req, misc_msgs::LawnmoverService::Response &res)
		{
			/*** Generate mission xml file ***/
			MG.writeXML.addMission();
			MG.generateRows(
					req.start_position.x,
					req.start_position.y,
					req.start_position.z,
					req.speed,
					req.victory_radius,
					req.width,
					req.length,
					req.horizontal_step,
					1.0, 			//md->alternation,
					0.0,			//md->coff,
					true,			//(md->flags & md->FLG_SQUARE_CURVE),
					req.bearing,	//md->bearing*180/M_PI,
					0.0,			//md->cross_angle,
					false);			//!(md->flags & md->FLG_CURVE_RIGHT));

			/*** Request mission execution ***/
			saveAndRequestAction("lawnmover");

			res.status = true;
			return true;
		}

		/*** Service that stops mission execution ***/
		bool Commander::stopService(std_srvs::Trigger::Request &req, std_srvs::Trigger::Response &res)
		{
			publishEventString("/STOP");
			return true;
		}

		/*** Service that pauses mission execution ***/
		bool Commander::pauseService(std_srvs::Trigger::Request &req, std_srvs::Trigger::Response &res)
		{
			publishEventString("/PAUSE");
			return true;
		}

		/*** Service that continues mission execution ***/
		bool Commander::continueService(std_srvs::Trigger::Request &req, std_srvs::Trigger::Response &res)
		{
			publishEventString("/CONTINUE");
			return true;
		}

		/*****************************************************************
		 ***  Helper functions
		 ****************************************************************/

		void Commander::publishStartParser(std::string xml)
		{
			misc_msgs::StartParser msg;
			msg.method = misc_msgs::StartParser::FILENAME;
			msg.missionData = xml.c_str();
			pubStartParser.publish(msg);
		}

		void Commander::publishEventString(std::string event)
		{
			std_msgs::String msg;
			msg.data = event.c_str();
			pubEventString.publish(msg);
		}

		void Commander::saveAndRequestAction(std::string name)
		{
			std::string filename = xml_save_path+name+"_"+boost::lexical_cast<std::string>(ros::Time::now())+".xml";
			MG.writeXML.saveXML(filename.c_str());
			publishStartParser(filename.c_str());
			publishEventString("/START_PARSER");
			ROS_INFO("Commander: %s request",name.c_str());
		}
	}
}

#endif /* LABUST_ROS_PKG_LABUST_MISSION_INCLUDE_LABUST_MISSION_COMMANDER_H_ */