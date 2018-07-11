/* 
 *            Copyright 2009-2018 The VOTCA Development Team
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


#include <votca/xtp/polarsite.h>
#include <boost/math/special_functions/round.hpp>
#include <boost/format.hpp>
#include <fstream>
#include <string>
#include <votca/tools/constants.h>


using namespace std;

namespace votca {
  namespace xtp {

    void PolarSite::calcRank() {
      // Get Rank: We have at the moment just three cases. Point charges, dipoles and Quadrupoles
      //It calculates the rank in the spherical case
      int mplen = _multipole.size();
      if (mplen == 1) {
        _rank = 0;
      } else if (mplen == 4) {
        _rank = 1;
      } else if (mplen == 9) {
        _rank = 2;
      } else {
        throw std::runtime_error("PolarSite. multipoles size is not 1,4 or 9.");
      }
      return;
    }

    Eigen::Matrix3d PolarSite::CalculateCartesianMultipole() {
      // spherical_multipoles Q = ( Q00,Q10,Q11c,Q11s,Q20,Q21c,Q21s,Q22c,Q22s )
      // We are trasforming here just quadrupoles
      const Eigen::VectorXd& MP = _multipole;
      if (_rank > 1) {
        Eigen::Matrix3d theta;
        double sqr3 = std::sqrt(3);
        theta(0, 0) = 0.5 * (-MP(4) + sqr3 * MP(7)); // theta_xx
        theta(1, 1) = 0.5 * (-MP(4) + sqr3 * (-MP(7))); // theta_yy
        theta(2, 2) = MP(4); // theta_zz
        theta(0, 1) = theta(1, 0) = 0.5 * sqr3 * MP(8); // theta_xy = theta_yx
        theta(0, 2) = theta(2, 0) = 0.5 * sqr3 * MP(5); // theta_xz = theta_zx
        theta(1, 2) = theta(2, 1) = 0.5 * sqr3 * MP(6); //theta_yz = theta_zy 
        return theta;
      }
    }

    Eigen::VectorXd PolarSite::CalculateSphericalMultipole(const Eigen::Matrix3d& _quadrupole_cartesian) {
      Eigen::Matrix3d theta = _quadrupole_cartesian;
      Eigen::VectorXd quadrupole_polar = Eigen::VectorXd::Zero(4);
      double sqr3 = std::sqrt(3);
      quadrupole_polar(0) = theta(2, 2);
      quadrupole_polar(1) = (2. / sqr3) * theta(0, 2);
      quadrupole_polar(2) = (2. / sqr3) * theta(1, 2);
      quadrupole_polar(3) = (1. / sqr3) * (theta(0, 0) - theta(1, 1));
      quadrupole_polar(4) = (2. / sqr3) * theta(0, 1);
      return quadrupole_polar;
    }

    void PolarSite::Rotate(const Eigen::Matrix3d& R,const Eigen::Vector3d &refPos) {
      Translate(-refPos);
      _pos=R*_pos;
      Translate(refPos); //Rotated Position

      
        if (_rank > 0) {
          _multipole.segment<3>(1) = R * _multipole.segment<3>(1);
        }
        if (_rank > 1) {
          Eigen::Matrix3d cartesianquad = CalculateCartesianMultipole();
          Eigen::Matrix3d rotated = R * cartesianquad * R.transpose();
          _multipole.segment<5>(4) = CalculateSphericalMultipole(rotated);
        }
      
      _Ps=R*_Ps*R.transpose();
      
      return;
    }

    void PolarSite::Translate(const Eigen::VectorXd &shift) {
      _pos += shift;
      return;
    }

    void PolarSite::Induce(double wSOR) {
      // SUCCESSIVE OVERRELAXATION
      _inducedDipole_old = _inducedDipole; // Remember all previous moments
      _inducedDipole = (1 - wSOR) * _inducedDipole_old - wSOR*_Ps * (_localpermanetField + _localinducedField);
      return;
    }

    Eigen::MatrixXd PolarSite::FillInteraction(const PolarSite& otherSite) {

      const Eigen::Vector3d& posB = otherSite.getPos();
      const Eigen::Vector3d& posA = this->getPos();
      int rankA = this->getRank();
      int rankB = otherSite.getRank();
      int size1 = this->getPermMultipole().size();
      if (this->isPolarisable() && rankA<1) {
        rankA=1;
        size1=4;
      }
      int size2 = otherSite.getPermMultipole().size();
      if (otherSite.isPolarisable() && rankB<1) {
        rankB=1;
        size2=4;
      }
      
      Eigen::MatrixXd interaction = Eigen::MatrixXd::Zero(size1, size2);
      const Eigen::Vector3d r_AB = posB - posA; //Vector of the distance between polar sites
      const double R = r_AB.norm(); //Norm of distance vector
      const Eigen::Vector3d pos_a = r_AB / R; //unit vector on the sites reciprocal direction; This points toward A
      const Eigen::Vector3d pos_b = -pos_a; //unit vector on the sites reciprocal direction; This points toward B   

      const double fac0 = 1 / R;
      
      //Charge-Charge Interaction
      interaction(0, 0) = fac0; //T_00,00
      const double sqr3 = std::sqrt(3);
      const Eigen::Matrix3d AxA = pos_a * pos_a.transpose();
      const Eigen::Matrix3d BxB = pos_b * pos_b.transpose();

      const double fac1 = std::pow(fac0, 2);
      const double fac2 = std::pow(fac0, 3);

      if (rankA > 0) {
        //Dipole-Charge Interaction
        interaction.block<3, 1>(1, 0) = fac1*pos_a; //T_1alpha,00 (alpha=x,y,z)
      }
      if (rankA > 1) {
        //Quadrupole-Charge interaction
        interaction(4, 0) = fac2 * 0.5 * (3 * AxA(2, 2) - 1); //T20,00
        interaction(5, 0) = fac2 * sqr3 * AxA(0, 2); //21c,00
        interaction(6, 0) = fac2 * sqr3 * AxA(1, 2); //T21s,000
        interaction(7, 0) = fac2 * 0.5 * sqr3 * (AxA(0, 0) - AxA(1, 1)); //T22c,00
        interaction(8, 0) = fac2 * sqr3 * AxA(0, 1); //T22s,00
      }

      if (rankB > 0 ) {
        //Charge-Dipole Interaction
        interaction.block<1, 3>(0, 1) = fac1*pos_b; //T_00,1alpha (alpha=x,y,z)
      }
      if (rankB > 1) {
        //Charge-Quadrupole Interaction
        interaction(0, 4) = fac2 * 0.5 * (3 * BxB(2, 2) - 1); //T00,20
        interaction(0, 5) = fac2 * sqr3 * BxB(0, 2); //T00,21c
        interaction(0, 6) = fac2 * sqr3 * BxB(1, 2); //T00,21s
        interaction(0, 7) = fac2 * 0.5 * sqr3 * (BxB(0, 0) - BxB(1, 1)); //T00,22c
        interaction(0, 8) = fac2 * sqr3 * BxB(0, 1); //T00,22s
      }

      const Eigen::Matrix3d c = Eigen::Matrix3d::Identity();
      if (rankA > 0 && rankB > 0 ){
        const Eigen::Matrix3d AxB = pos_a * pos_b.transpose();
        //Dipole-Dipole Interaction 
        interaction.block<3, 3>(1, 1) = fac2 * (3 * AxB); //T_1alpha,1beta (alpha,beta=x,y,z) 
      const double fac3 = std::pow(fac0, 4);
      if (rankA > 1) {
        //Quadrupole-Dipole Interaction
        interaction.block<1, 3>(4, 1) = 0.5 * fac3 * (15 * AxA(2, 2) * pos_b.transpose() + 6 * pos_a(2) * c.row(2) - 3 * pos_b.transpose()); //T20-1beta (beta=x,y,z)
        interaction.block<1, 3>(5, 1) = fac3 * sqr3 * (pos_a(0) * c.row(2) + c.row(0) * pos_a(2) + 5 * AxA(0, 2) * pos_b.transpose()); //T21c-1beta (beta=x,y,z)
        interaction.block<1, 3>(6, 1) = fac3 * sqr3 * (pos_a(1) * c.row(2) + c.row(1) * pos_a(2) + 5 * AxA(1, 2) * pos_a.transpose()); //T21s-1beta (beta=x,y,z)
        interaction.block<1, 3>(7, 1) = fac3 * 0.5 * sqr3 * (5 * (AxA(0, 0) - AxA(1, 1)) * pos_b.transpose() + 2 * pos_a(0) * c.row(0) - 2 * pos_a(1) * c.row(1)); //T22c-1beta (beta=x,y,z)
        interaction.block<1, 3>(8, 1) = fac3 * sqr3 * (5 * AxA(0, 1) * pos_b.transpose() + pos_a(0) * c.row(1) + pos_a(1) * c.row(0)); //T22s-1beta (beta=x,y,z)    
      }
      if (rankB > 1) {
        //Dipole-Quadrupole Interaction
        interaction.block<3, 1>(1, 4) = 0.5 * fac3 * (15 * BxB(2, 2) * pos_a + 6 * pos_b(2) * c.col(2) - 3 * pos_a); //T1beta-20 (beta=x,y,z)
        interaction.block<3, 1>(1, 5) = fac3 * sqr3 * (pos_b(0) * c.col(2) + c.col(0) * pos_b(2) + 5 * BxB(0, 2) * pos_a); //T1beta-21c (beta=x,y,z)
        interaction.block<3, 1>(1, 6) = fac3 * sqr3 * (pos_b(1) * c.col(2) + c.col(1) * pos_b(2) + 5 * BxB(1, 2) * pos_b); //T1beta-21s (beta=x,y,z)
        interaction.block<3, 1>(1, 7) = 0.5 * fac3 * sqr3 * (5 * (BxB(0, 0) - BxB(1, 1)) * pos_a + 2 * pos_b(0) * c.col(0) - 2 * pos_b(1) * c.col(1)); //T1beta-22c (beta=x,y,z)
        interaction.block<3, 1>(1, 8) = fac3 * sqr3 * (5 * BxB(0, 1) * pos_a + pos_b(0) * c.col(1) + pos_b(1) * c.col(0)); //T1beta-22s (beta=x,y,z)    
      }

      if (rankA > 1 && rankB > 1) {
        const double fac4 = std::pow(fac0, 5);
        //Quadrupole-Quadrupole Interaction
        interaction(4, 4) = fac4 * (3. / 4.)*(35 * AxA(2, 2) * BxB(2, 2) - 5 * AxA(2, 2) - 5 * BxB(2, 2) + 20 * AxB(2, 2) * c(2, 2) + 2 * c(2, 2) * c(2, 2) + 1); //T20,20

        interaction(4, 5) = 0.5 * fac4 * sqr3 * (35 * AxA(2, 2) * BxB(0, 2) - 5 * BxB(0, 2) + 10 * AxB(2, 0) * c(2, 2) + 10 * AxB(2, 2) * c(2, 1) + 2 * c(2, 0) * c(2, 2)); //T20,21c

        interaction(4, 6) = 0.5 * fac4 * sqr3 * (35 * AxA(2, 2) * BxB(1, 2) - 5 * BxB(1, 2) + 10 * AxB(2, 1) * c(2, 2) + 10 * AxB(2, 2) * c(2, 1) + 2 * c(2, 1) * c(2, 2)); //T20,21s

        interaction(4, 7) = 0.25 * fac4 * sqr3 * (35 * AxB(2, 0) - 35 * AxB(2, 1) - 5 * BxB(0, 0) + 5 * BxB(1, 1) + 20 * AxB(2, 0) * c(2, 0) - 20 * AxB(2, 1) * c(2, 1) + 2 * c(2, 0) * c(2, 0) - 2 * c(2, 1) * c(2, 1)); //T20,22c

        interaction(4, 8) = 0.5 * fac4 * sqr3 * (35 * AxA(2, 2) * BxB(0, 1) - 5 * BxB(0, 1) + 10 * AxB(2, 0) * c(2, 1) + 10 * AxB(2, 1) * c(2, 0) + 2 * c(2, 0) * c(2, 1)); //T20,22s

        interaction(5, 5) = fac4 * (35 * AxA(0, 2) * BxB(0, 2) + 5 * AxB(0, 0) * c(2, 2) + 5 * AxB(0, 2) * c(2, 0) + 5 * AxB(2, 0) * c(0, 2) + 5 * AxB(2, 2) * c(0, 0) + c(0, 0) * c(2, 2) + c(0, 2) * c(2, 0)); //T21c,21c

        interaction(5, 6) = fac4 * (35 * AxA(0, 2) * BxB(1, 2) + 5 * AxB(0, 1) * c(2, 2) + 5 * AxB(0, 2) * c(2, 1) + 5 * AxB(2, 1) * c(0, 2) + 5 * AxB(2, 2) * c(0, 1) + c(0, 1) * c(2, 2) + c(0, 2) * c(2, 1)); //T21c,21s

        interaction(5, 7) = 0.5 * fac4 * (35 * AxA(0, 2) * BxB(0, 0) - 35 * AxA(0, 2) * BxB(1, 1) + 10 * AxB(0, 0) * c(2, 0) - 10 * AxB(0, 1) * c(2, 1) + 10 * AxB(0, 0) * c(0, 0) - 10 * AxB(2, 1) * c(0, 1) + 2 * c(0, 0) * c(2, 0) - 2 * c(0, 1) * c(2, 1)); //T21c,22c

        interaction(5, 8) = fac4 * (35 * AxA(0, 2) * BxB(0, 1) + 5 * AxB(0, 0) * c(2, 1) + 5 * AxB(0, 1) * c(2, 0) + 5 * AxB(2, 0) * c(0, 1) + 5 * AxB(2, 1) * c(0, 0) + c(0, 0) * c(2, 1) + c(0, 1) * c(2, 0)); //T21c,22s

        interaction(6, 6) = fac4 * (35 * AxA(1, 2) * BxB(1, 2) + 5 * AxB(1, 1) * c(2, 2) + 5 * AxB(1, 2) * c(2, 1) + 5 * AxB(2, 1) * c(1, 2) + 5 * AxB(2, 2) * c(1, 1) + c(1, 1) * c(2, 2) + c(1, 2) * c(2, 1)); //T21s,21s

        interaction(6, 7) = 0.5 * fac4 * (35 * AxA(1, 2) * BxB(0, 0) - 35 * AxA(1, 2) * BxB(1, 1) + 10 * AxB(1, 2) * c(2, 0) - 10 * AxB(1, 1) * c(2, 1) + 10 * AxB(2, 0) * c(1, 0) - 10 * AxB(2, 1) * c(1, 1) + 2 * c(1, 0) * c(2, 0) - 2 * c(1, 1) * c(2, 1)); //T21s,22c

        interaction(6, 8) = fac4 * (35 * AxA(1, 2) * BxB(0, 1) + 5 * AxB(1, 0) * c(2, 1) + 5 * AxB(1, 1) * c(2, 1) + 5 * AxB(2, 0) * c(1, 1) + 5 * AxB(2, 1) * c(1, 2) + c(1, 0) * c(2, 1) + c(1, 1) * c(2, 0)); //T21s,22s

        interaction(7, 7) = 0.25 * fac4 * (35 * AxA(0, 0) * BxB(0, 0) - 35 * AxA(0, 0) * BxB(1, 1) - 35 * AxA(1, 1) * BxB(0, 0) + 35 * AxA(1, 1) * BxB(1, 1) + 20 * AxB(0, 0) * c(0, 0) - 20 * AxB(0, 1) * c(0, 1) - 20 * AxB(1, 0) * c(1, 0)
                + 20 * AxB(0, 0) * c(1, 1) + 2 * c(0, 0) * c(0, 0) - 2 * c(0, 1) * c(0, 1) - 2 * c(1, 0) * c(1, 0) + 2 * c(1, 1) * c(1, 1)); //T22c,22c

        interaction(7, 8) = 0.5 * fac4 * (35 * BxB(0, 1) * AxA(0, 0) - 35 * BxB(1, 2) * AxA(1, 1) + 10 * AxB(0, 0) * c(0, 1) + 10 * AxB(0, 1) * c(0, 0) - 10 * AxB(1, 0) * c(1, 1) - 10 * AxB(1, 1) * c(1, 2) + 2 * c(0, 0) * c(0, 1) - 2 * c(1, 0) * c(1, 1)); //T22c,22s

        interaction(8, 8) = 0.5 * fac4 * (35 * AxA(0, 1) * BxB(0, 1) + 5 * AxB(0, 0) * c(1, 1) + 5 * AxB(0, 1) * c(1, 0) + 5 * AxB(1, 0) * c(0, 1) + 5 * AxB(1, 1) * c(0, 0) + c(0, 0) * c(1, 1) + c(0, 1) * c(1, 0)); //T22s,22s

        interaction(5, 4) = 0.5 * fac4 * sqr3 * (35 * BxB(2, 2) * AxA(0, 2) - 5 * AxA(0, 2) + 10 * AxB(0, 2) * c(2, 2) + 10 * AxB(2, 2) * c(1, 2) + 2 * c(0, 2) * c(2, 2)); //T21c,20

        interaction(6, 4) = 0.5 * fac4 * sqr3 * (35 * BxB(2, 2) * AxA(1, 2) - 5 * AxA(1, 2) + 10 * AxB(1, 2) * c(2, 2) + 10 * AxB(2, 2) * c(1, 2) + 2 * c(1, 2) * c(2, 2)); //T21s,20

        interaction(6, 5) = fac4 * (35 * BxB(0, 2) * AxA(1, 2) + 5 * AxB(1, 0) * c(2, 2) + 5 * AxB(2, 0) * c(1, 2) + 5 * AxB(2, 1) * c(2, 0) + 5 * AxB(2, 2) * c(1, 0) + c(1, 0) * c(2, 2) + c(2, 0) * c(1, 2)); //T21s,21c

        interaction(7, 4) = 0.25 * fac4 * sqr3 * (35 * AxB(0, 2) - 35 * BxB(2, 2) * AxA(1, 1) - 5 * AxA(0, 0) + 5 * AxA(1, 1) + 20 * AxB(0, 2) * c(0, 2) - 20 * AxB(1, 2) * c(1, 2) + 2 * c(0, 2) * c(0, 2) - 2 * c(1, 2) * c(1, 2)); //T22c,20

        interaction(7, 5) = 0.5 * fac4 * (35 * BxB(0, 2) * AxA(0, 0) - 35 * BxB(0, 2) * AxA(1, 1) + 10 * AxB(0, 0) * c(0, 2) - 10 * AxB(1, 0) * c(1, 2) + 10 * AxB(0, 0) * c(0, 0) - 10 * AxB(1, 2) * c(1, 0) + 2 * c(0, 0) * c(0, 2) - 2 * c(1, 0) * c(1, 2)); //T22c,21c

        interaction(7, 6) = 0.5 * fac4 * (35 * BxB(1, 2) * AxA(0, 0) - 35 * BxB(1, 2) * AxA(1, 1) + 10 * AxB(2, 1) * c(0, 2) - 10 * AxB(1, 1) * c(1, 2) + 10 * AxB(0, 2) * c(0, 1) - 10 * AxB(1, 2) * c(1, 1) + 2 * c(0, 1) * c(0, 2) - 2 * c(1, 1) * c(1, 2)); //T22c,21s

        interaction(8, 4) = 0.5 * fac4 * sqr3 * (35 * BxB(2, 2) * AxA(0, 1) - 5 * AxA(0, 1) + 10 * AxB(0, 2) * c(1, 2) + 10 * AxB(1, 2) * c(0, 2) + 2 * c(0, 2) * c(1, 2)); //T22s,20

        interaction(8, 5) = fac4 * (35 * BxB(0, 2) * AxA(1, 0) + 5 * AxB(0, 0) * c(1, 2) + 5 * AxB(1, 0) * c(0, 2) + 5 * AxB(0, 2) * c(1, 0) + 5 * AxB(1, 2) * c(0, 0) + c(0, 0) * c(1, 2) + c(1, 0) * c(0, 2)); //T22s,21c

        interaction(8, 6) = fac4 * (35 * BxB(1, 2) * AxA(0, 1) + 5 * AxB(0, 1) * c(1, 2) + 5 * AxB(1, 1) * c(1, 2) + 5 * AxB(0, 2) * c(1, 1) + 5 * AxB(1, 2) * c(2, 1) + c(0, 1) * c(1, 2) + c(1, 1) * c(0, 2)); //T22s,21s

        interaction(8, 7) = 0.5 * fac4 * (35 * AxA(1, 0) * BxB(0, 0) - 35 * AxA(1, 2) * BxB(1, 1) + 10 * AxB(0, 0) * c(1, 0) + 10 * AxB(1, 0) * c(0, 0) - 10 * AxB(0, 1) * c(1, 1) - 10 * AxB(1, 1) * c(2, 1) + 2 * c(0, 0) * c(1, 0) - 2 * c(0, 1) * c(1, 1)); //T22s,22c          
      }

      }
      return interaction; //in units of 4piepsilon0  
    }

    double PolarSite::InteractStatic(PolarSite& otherSite) {

      const Eigen::MatrixXd Tab = FillInteraction(otherSite); // T^(ab)_tu
      const Eigen::VectorXd& multipolesA = this->getPermMultipole(); //Q^(a)_t
      const Eigen::VectorXd& multipolesB = otherSite.getPermMultipole(); //Q^(b)_u  

      //Potentials
      this->PhiP+=(Tab.row(0)*multipolesB).sum();
      otherSite.PhiP+=(multipolesA.transpose()*Tab.col(0)).sum();
      //Fields
      if(this->isPolarisable()){
        this->_localpermanetField+=(Tab.block(1,0,3,Tab.cols())*multipolesB).rowwise().sum();
      }
      if(otherSite.isPolarisable()){
        otherSite._localpermanetField+=(multipolesA.transpose()*Tab.block(0,1,Tab.rows(),3)).colwise().sum().transpose();
      }
      //Energy
      const double EnergyAB = multipolesA.transpose() * Tab*multipolesB; //Interaction Energy between sites A and B

      return EnergyAB;
    }



    
    void PolarSite::WriteMpsLine(std::ostream &out, const string& unit) {
        double conv_pos = 1.;
        if (unit == "angstrom") {
            conv_pos = tools::conv::bohr2ang;
        }
        else if (unit == "bohr") {
            conv_pos = 1.;
        }
        else assert(false); // Units error
    
        out << (boost::format(" %1$2s %2$+1.7f %3$+1.7f %4$+1.7f Rank %5$d\n") 
                % _name % (_pos(0)*conv_pos)
                % (_pos(1)*conv_pos) % (_pos(2)*conv_pos)
                % _rank);
        out << (boost::format("    %1$+1.7f\n") % getCharge());
        if (_rank > 0) {
            // Dipole z x y
            out << (boost::format("    %1$+1.7f %2$+1.7f %3$+1.7f\n") 
                % _multipole(1) % _multipole(2) % _multipole(3));
            if (_rank > 1) {
                // Quadrupole 20 21c 21s 22c 22s
                out << (boost::format("    %1$+1.7f %2$+1.7f %3$+1.7f %4$+1.7f %5$+1.7f\n") 
                    % _multipole(4) % _multipole(5) % _multipole(6) 
                    % _multipole(7) % _multipole(8));
            }
        }
        // Polarizability
        out << (boost::format("     P %1$+1.7f %2$+1.7f %3$+1.7f %4$+1.7f %5$+1.7f %6$+1.7f \n") 
            % _Ps(0,0) % _Ps(1,0) % _Ps(2,0) % _Ps(1,1) % _Ps(1,2) % _Ps(2,2));   
    }



  }
}
