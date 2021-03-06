/*********************************************************************
 * Software License Agreement (BSD License)
 *
 *  Copyright (c) 2010-2015, LABUST, UNIZG-FER
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
#ifndef ALLOCATION_X2D_VP_QP_H
#define ALLOCATION_X2D_VP_QP_H
#include <labust/allocation/allocation_interface.h>
#include <labust/allocation/thruster_configuration.h>
#include <labust/allocation/xconfiguration.h>

#include <std_msgs/Float32.h>
#include <ros/ros.h>

#include <boost/array.hpp>

#include <Eigen/Dense>

#include <CGAL/basic.h>
#include <CGAL/QP_models.h>
#include <CGAL/QP_functions.h>
#include <CGAL/Gmpz.h>
#include <CGAL/MP_Float.h>

#include <cmath>
#include <vector>

namespace labust
{
	namespace allocation
	{
		/**
		 * The class implements a qudratic programming approach to allocation.
		 * The thruster number is assumed to be 6; 4 distributed in an X-configuration
		 * and 2 in vertical, one on fore and one on aft.
		 *
		 * The number of DoF is assumed to be 5; X,Y,Z,M,N.
		 * No assumption is made on thruster symmetry.
		 */
		class X2dVPQP : public virtual AllocationInterface
		{
			//Easy type handling for CGAL
			typedef CGAL::Gmpzf ET;
			typedef CGAL::Quadratic_program<double> Program;
			typedef CGAL::Quadratic_program_solution<ET> Solution;

			enum {X=0, Y, Z, M=4, N};
			enum {Xh=0,Yh,Nh};
			enum {Zv=0,Mv};

		public:
			///Main constructor
			X2dVPQP();

			/**
			 * The main configuration method. The method load parameters
			 * and initializes the subscribers and publishers of the allocation.
			 */
			bool configure(ros::NodeHandle& nh, ros::NodeHandle& ph);

			/**
			 * The main allocation function that distributes desired
			 * forces and moments directly to the thrusters.
			 *
			 * \return Returns the vector of PWM values need to achieve the desired tau
			 */
			const std::vector<double>& allocate(const Eigen::VectorXd& tau);

			/**
			 * The method returns the last achieved tau vector.
			 */
			const Eigen::VectorXd& tauA() const {return tau_ach;};

			/**
			 * The method returns the windup flags per degree of freedom.
			 */
			const std::vector<bool>& windup(){return _windup;};

		private:
			///Helper function for coercing XY in-place
			template <class TypeV, class TypeMinMax>
			bool saturate(TypeV& t, const TypeMinMax& pmin, const TypeMinMax& pmax);

			///Helper class for the X configuration
			XConfiguration xconf;
			///Quadratic Program solver
			Program qp;

			///The thruster configuration
			ThrusterConfiguration thrusters;

			///The windup flags
			std::vector<bool> _windup;

			///Minimum guaranteed yaw torque
			double minN;
			///The final achieved tau
			Eigen::VectorXd tau_ach;
		};
	}
}

/* ALLOCATION_X2D_VP_QP_H */
#endif
