/* 
 *            Copyright 2009-2017 The VOTCA Development Team
 *                       (http://www.votca.org)
 *
 *      Licensed under the Apache License, Version 2.0 (the "License")
 *
 * You may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *              http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#ifndef __XTP_GEOMETRY_OPTIMIZATION__H
#define	__XTP_GEOMETRY_OPTIMIZATION__H

// Overload of uBLAS prod function with MKL/GSL implementations
#include <votca/tools/linalg.h>
#include <boost/numeric/ublas/operation.hpp>
#include <votca/ctp/qmatom.h>
#include <votca/ctp/logger.h>
#include <votca/ctp/segment.h>
#include <stdio.h>


using namespace std;

namespace votca { namespace xtp {

    namespace ub = boost::numeric::ublas;
    
    

        class GeometryOptimization {
        public: 
            
            GeometryOptimization():_iteration(0) {};
            ~GeometryOptimization(){};

            void BFGSStep( int& _iteration, bool& _update_hessian,  ub::matrix<double>& _force, ub::matrix<double>& _force_old,  ub::matrix<double>& _current_xyz, ub::matrix<double>&  _old_xyz, ub::matrix<double>& _hessian ,ub::matrix<double>& _xyz_shift ,ub::matrix<double>& _trial_xyz  );
            int Iteration(){ return _iteration;};
            void Initialize( int natoms );
            void Checkpoint( std::vector<ctp::Segment* >& _molecule );
            void WriteIteration( FILE* out, ctp::Segment* _segment );



            
            
        private:
            
            int _natoms;
            int _iteration;
            ub::matrix<double> _force;
            ub::matrix<double> _force_old;
            ub::matrix<double> _xyz_shift;
            ub::matrix<double> _speed;
            ub::matrix<double> _current_xyz;
            ub::matrix<double> _old_xyz; 
            ub::matrix<double> _trial_xyz; 
            ub::matrix<double> _hessian;
            
            bool _step_accepted;
            bool _update_hessian;
            
            double _trust_radius;
            double _norm_delta_pos;
            double _delta_energy_estimate;
            
            
            
        };

    }}
#endif	/* GEOMETRY_OPTIMIZATION_H */
