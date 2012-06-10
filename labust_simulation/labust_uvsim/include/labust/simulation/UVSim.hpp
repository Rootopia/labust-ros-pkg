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
*********************************************************************/
#ifndef UVSIM_HPP_
#define UVSIM_HPP_

#include <labust/xml/xmlfwd.hpp>
#include <labust/simulation/matrixfwd.hpp>
#include <labust/vehicles/VehicleDriver.hpp>
#include <labust/xml/adapt_class.hpp>

#include <string>

namespace labust
{
	namespace simulation
	{
		class VehicleModel6DOF;
		/**
		 * This class implements a VideoRay simulator. It supports XML configuration of the dynamic, noise and environment model.
		 *
		 * \todo Add state outputs configuration.
		 */
		class UVSim : public virtual labust::vehicles::Driver
		{
			enum {N=0, E=1, D=2};
			enum {current_x=0, current_y=1, current_z=2};

		public:
			/**
			 * Main constructor. Takes a XML reader pointer and configures the models.
			 *
			 * \param reader Pointer to the XMLReader object cointaining the configuration data.
			 * \param id Identification class.
			 */
			UVSim(const labust::xml::ReaderPtr reader, const std::string& id = "");

			/**
			 * Implementation of labust::vehicles::Driver::setTAU.
			 */
			void setTAU(const labust::vehicles::tauMapRef tau);
			/**
			 * Implementation of labust::vehicles::Driver::getState
			 */
			void getState(labust::vehicles::stateMapRef state);
			/**
			 * Implementation of labust::vehicles::Driver::setGuidance
			 */
			void setGuidance(const labust::vehicles::guidanceMapRef guidance);
			/**
			 * Implementation of labust::vehicles::Driver::setCommand
			 */
			void setCommand(const labust::apps::stringRef commands);
			/**
			 * Implementation of labust::vehicles::Driver::getData
			 */
			void getData(labust::apps::stringPtr data);

			PP_LABUST_IN_CLASS_ADAPT_TO_XML(UVSim)

		private:
			/**
			 * Dynamics and kinematics model.
			 */
			boost::shared_ptr<VehicleModel6DOF> model;
			/**
			 * Current vector.
			 */
			vector3 currentForce, current;
		};

		PP_LABUST_MAKE_CLASS_XML_OPERATORS(,UVSim,
				(vector3,currentForce)
				(vector3,current))
	}
}
/* UVSIM_HPP_ */
#endif
