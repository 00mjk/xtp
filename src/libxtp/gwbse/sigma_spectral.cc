/*
 *            Copyright 2009-2019 The VOTCA Development Team
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

#include <votca/xtp/sigma_spectral.h>
#include <votca/xtp/rpa.h>
#include <votca/xtp/threecenter.h>
#include "votca/xtp/vc2index.h"

namespace votca {
  namespace xtp {

    void Sigma_Spectral::PrepareScreening() {
      // Solve eigenvalue problem
      _EigenSol = _rpa.calculate_eigenvalues();
      // Cache residues
      const int numeigenvalues = _EigenSol._Omega.size();
      _residues.resize(numeigenvalues);
      for (int s = 0; s < numeigenvalues; s++) {
        _residues[s] = CalcResidues(s);
      }
      return;
    }

    Eigen::VectorXd Sigma_Spectral::CalcCorrelationDiag(const Eigen::VectorXd& frequencies) const {
      const int numeigenvalues = _EigenSol._Omega.size();
      Eigen::VectorXd result = Eigen::VectorXd::Zero(_qptotal);

      if (_HedinApprox) {
        
        for (int s = 0; s < numeigenvalues; s++) {
          double omega = _EigenSol._Omega(s);
          const Eigen::MatrixXd& residues = _residues[s];
          for (int n = 0; n < _qptotal; n++) {
            Eigen::VectorXd rn_x_rn = residues.col(n).cwiseAbs2();
            result(n) += Equation48(rn_x_rn, omega);
          } // Energy level m
        } // Eigenvalues/poles s

      } else {

        for (int s = 0; s < numeigenvalues; s++) {
          double omega = _EigenSol._Omega(s);
          const Eigen::MatrixXd& residues = _residues[s];
          for (int n = 0; n < _qptotal; n++) {
            Eigen::VectorXd rn_x_rn = residues.col(n).cwiseAbs2();
            result(n) += Equation47(rn_x_rn, omega, frequencies(n));
          } // Energy level m
        } // Eigenvalues/poles s

      }
      
      return result;
    }

    Eigen::MatrixXd Sigma_Spectral::CalcCorrelationOffDiag(const Eigen::VectorXd& frequencies) const {
      const int numeigenvalues = _EigenSol._Omega.size();
      Eigen::MatrixXd result = Eigen::MatrixXd::Zero(_qptotal, _qptotal);
      
      if (_HedinApprox) {

        for (int s = 0; s < numeigenvalues; s++) {
          double omega = _EigenSol._Omega(s);
          const Eigen::MatrixXd& residues = _residues[s];
          for (int m = 0; m < _qptotal; m++) {
            for (int n = m + 1; n < _qptotal; n++) {
              Eigen::VectorXd rm_x_rn = residues.col(m).cwiseProduct(residues.col(n));
              double res = Equation48(rm_x_rn, omega);
              result(m, n) += res;
              result(n, m) += res;
            } // Energy level n
          } // Energy level m
        } // Eigenvalues/poles s

      } else {

        for (int s = 0; s < numeigenvalues; s++) {
          double omega = _EigenSol._Omega(s);
          const Eigen::MatrixXd& residues = _residues[s];
          for (int m = 0; m < _qptotal; m++) {
            for (int n = m + 1; n < _qptotal; n++) {
              Eigen::VectorXd rm_x_rn = residues.col(m).cwiseProduct(residues.col(n));
              double result_m = Equation47(rm_x_rn, omega, frequencies(m));
              double result_n = Equation47(rm_x_rn, omega, frequencies(n));
              // (m|S(w)|n) = 0.5 * (m|S(e_m)|n) + 0.5 * (m|S(e_n)|n)
              double res = 0.5 * (result_m + result_n);
              result(m, n) += res;
              result(n, m) += res;
            } // Energy level n
          } // Energy level m
        } // Eigenvalues/poles s

      }

      return result;
    }

    Eigen::MatrixXd Sigma_Spectral::CalcResidues(int s) const {
      const int lumo = _opt.homo + 1;
      const int n_occup = lumo - _opt.rpamin;
      const int n_unocc = _opt.rpamax - _opt.homo;
      const int qpoffset = _opt.qpmin - _opt.rpamin;
      const int auxsize = _Mmn.auxsize(); // Size of gwbasis
      const Eigen::VectorXd& xpy = _EigenSol._XpY.col(s);
      vc2index vc = vc2index(0, 0, n_unocc);
      Eigen::MatrixXd residues = Eigen::MatrixXd::Zero(_rpatotal, _qptotal);
      
      for (int v = 0; v < n_occup; v++) {
        const Eigen::MatrixXd Mmn_vT =
                _Mmn[v].block(n_occup, 0, n_unocc, auxsize).transpose();
        const Eigen::VectorXd xpyv = xpy.segment(vc.I(v, 0), n_unocc);
        for (int m = 0; m < _qptotal; m++) {
          const Eigen::MatrixXd fc =
                  _Mmn[m + qpoffset] * Mmn_vT;
          residues.col(m) += fc * xpyv;
        } // MO m
      } // Occupied MO v

      return residues;
    }

    double Sigma_Spectral::Equation47(const Eigen::VectorXd& A12, double omega, double frequency) const {
      const double eta = 1e-4;
      const int lumo = _opt.homo + 1;
      const int n_occup = lumo - _opt.rpamin;
      const int n_unocc = _opt.rpamax - _opt.homo;
      Eigen::ArrayXd B12 = -_rpa.getRPAInputEnergies().array() + frequency;
      B12.segment(0, n_occup) += omega;
      B12.segment(n_occup, n_unocc) -= omega;
      const Eigen::ArrayXd numer = A12.array() * B12;
      const Eigen::ArrayXd denom = B12.abs2() + eta * eta;
      return (numer / denom).sum();
    }

    double Sigma_Spectral::Equation48(const Eigen::VectorXd& A12, double omega) const {
      const int lumo = _opt.homo + 1;
      const int n_occup = lumo - _opt.rpamin;
      double s1 = A12.head(n_occup).sum();
      double s2 = A12.sum();
      return 2 * (s1 - s2) / omega;
    }

  }
};
