/*
 *            Copyright 2009-2020 The VOTCA Development Team
 *                       (http://www.votca.org)
 *
 *      Licensed under the Apache License, Version 2.0 (the "License")
 *
 * You may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *              http://www.apache.org/licenses/LICENSE-2.0
 *
 *Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

//#include <votca/xtp/aopotential.h>
#include <votca/xtp/aomatrix.h>
#include <votca/xtp/aotransform.h>
#include <votca/xtp/qmmolecule.h>

namespace votca {
namespace xtp {

void AO3ddipole::FillBlock(std::vector<Eigen::Block<Eigen::MatrixXd>>& matrix,
                           const AOShell& shell_row,
                           const AOShell& shell_col) const {

  const double pi = boost::math::constants::pi<double>();

  Index rank = 1;

  // const double charge = _site->getCharge();
  const Eigen::Vector3d dipole = Eigen::Vector3d::Ones();

  // shell info, only lmax tells how far to go
  Index lmax_row = Index(shell_row.getL());
  Index lmax_col = Index(shell_row.getL());
  Index lsum = lmax_row + lmax_col;
  // set size of internal block for recursion
  Index nrows = AOTransform::getBlockSize(lmax_row);
  Index ncols = AOTransform::getBlockSize(lmax_col);

  std::array<int, 9> n_orbitals = AOTransform::n_orbitals();
  std::array<int, 165> nx = AOTransform::nx();
  std::array<int, 165> ny = AOTransform::ny();
  std::array<int, 165> nz = AOTransform::nz();
  std::array<int, 165> i_less_x = AOTransform::i_less_x();
  std::array<int, 165> i_less_y = AOTransform::i_less_y();
  std::array<int, 165> i_less_z = AOTransform::i_less_z();

  // get shell positions
  const Eigen::Vector3d& pos_row = shell_row.getPos();
  const Eigen::Vector3d& pos_col = shell_col.getPos();
  const Eigen::Vector3d diff = pos_row - pos_col;
  // initialize some helper

  double distsq = diff.squaredNorm();

  // iterate over Gaussians in this shell_row
  for (const auto& gaussian_row : shell_row) {

    // iterate over Gaussians in this shell_col
    // get decay constant
    const double decay_row = gaussian_row.getDecay();

    for (const auto& gaussian_col : shell_col) {
      // get decay constant

      const double decay_col = gaussian_col.getDecay();

      const double zeta = decay_row + decay_col;
      const double fak = 0.5 / zeta;
      const double fak2 = 2.0 * fak;
      const double xi = decay_row * decay_col * fak2;

      double exparg = xi * distsq;
      // check if distance between postions is big, then skip step
      if (exparg > 30.0) {
        continue;
      }

      // some helpers
      const Eigen::Vector3d PmA =
          fak2 * (decay_row * pos_row + decay_col * pos_col) - pos_row;

      const Eigen::Vector3d PmB =
          fak2 * (decay_row * pos_row + decay_col * pos_col) - pos_col;

      const Eigen::Vector3d PmC =
          fak2 * (decay_row * pos_row + decay_col * pos_col) - _r;

      const double U = zeta * PmC.squaredNorm();
      // +3 quadrupole, +2 dipole, +1 nuclear attraction integrals
      const Eigen::VectorXd FmU = AOTransform::XIntegrate(lsum + rank + 1, U);

      Eigen::Tensor<double, 3> nuc3(nrows, ncols, lsum + 1);
      nuc3.setZero();
      // (s-s element normiert )
      double prefactor = 4. * sqrt(2. / pi) * pow(decay_row * decay_col, .75) *
                         fak2 * exp(-exparg);

      //------------------------------------------------------
      //------------------------------------------------------

      // Integrals     p - s
      if (lmax_row > 0) {
        for (Index m = 0; m < lsum; m++) {
          nuc3(Cart::x, 0, m) =
              PmA(0) * nuc3(0, 0, m) - PmC(0) * nuc3(0, 0, m + 1);
          nuc3(Cart::y, 0, m) =
              PmA(1) * nuc3(0, 0, m) - PmC(1) * nuc3(0, 0, m + 1);
          nuc3(Cart::z, 0, m) =
              PmA(2) * nuc3(0, 0, m) - PmC(2) * nuc3(0, 0, m + 1);
        }
      }
      //------------------------------------------------------

      // Integrals     d - s
      if (lmax_row > 1) {
        for (Index m = 0; m < lsum - 1; m++) {
          double term = fak * (nuc3(0, 0, m) - nuc3(0, 0, m + 1));
          nuc3(Cart::xx, 0, m) = PmA(0) * nuc3(Cart::x, 0, m) -
                                 PmC(0) * nuc3(Cart::x, 0, m + 1) + term;
          nuc3(Cart::xy, 0, m) =
              PmA(0) * nuc3(Cart::y, 0, m) - PmC(0) * nuc3(Cart::y, 0, m + 1);
          nuc3(Cart::xz, 0, m) =
              PmA(0) * nuc3(Cart::z, 0, m) - PmC(0) * nuc3(Cart::z, 0, m + 1);
          nuc3(Cart::yy, 0, m) = PmA(1) * nuc3(Cart::y, 0, m) -
                                 PmC(1) * nuc3(Cart::y, 0, m + 1) + term;
          nuc3(Cart::yz, 0, m) =
              PmA(1) * nuc3(Cart::z, 0, m) - PmC(1) * nuc3(Cart::z, 0, m + 1);
          nuc3(Cart::zz, 0, m) = PmA(2) * nuc3(Cart::z, 0, m) -
                                 PmC(2) * nuc3(Cart::z, 0, m + 1) + term;
        }
      }
      //------------------------------------------------------

      // Integrals     f - s
      if (lmax_row > 2) {
        for (Index m = 0; m < lsum - 2; m++) {
          nuc3(Cart::xxx, 0, m) =
              PmA(0) * nuc3(Cart::xx, 0, m) -
              PmC(0) * nuc3(Cart::xx, 0, m + 1) +
              2 * fak * (nuc3(Cart::x, 0, m) - nuc3(Cart::x, 0, m + 1));
          nuc3(Cart::xxy, 0, m) =
              PmA(1) * nuc3(Cart::xx, 0, m) - PmC(1) * nuc3(Cart::xx, 0, m + 1);
          nuc3(Cart::xxz, 0, m) =
              PmA(2) * nuc3(Cart::xx, 0, m) - PmC(2) * nuc3(Cart::xx, 0, m + 1);
          nuc3(Cart::xyy, 0, m) =
              PmA(0) * nuc3(Cart::yy, 0, m) - PmC(0) * nuc3(Cart::yy, 0, m + 1);
          nuc3(Cart::xyz, 0, m) =
              PmA(0) * nuc3(Cart::yz, 0, m) - PmC(0) * nuc3(Cart::yz, 0, m + 1);
          nuc3(Cart::xzz, 0, m) =
              PmA(0) * nuc3(Cart::zz, 0, m) - PmC(0) * nuc3(Cart::zz, 0, m + 1);
          nuc3(Cart::yyy, 0, m) =
              PmA(1) * nuc3(Cart::yy, 0, m) -
              PmC(1) * nuc3(Cart::yy, 0, m + 1) +
              2 * fak * (nuc3(Cart::y, 0, m) - nuc3(Cart::y, 0, m + 1));
          nuc3(Cart::yyz, 0, m) =
              PmA(2) * nuc3(Cart::yy, 0, m) - PmC(2) * nuc3(Cart::yy, 0, m + 1);
          nuc3(Cart::yzz, 0, m) =
              PmA(1) * nuc3(Cart::zz, 0, m) - PmC(1) * nuc3(Cart::zz, 0, m + 1);
          nuc3(Cart::zzz, 0, m) =
              PmA(2) * nuc3(Cart::zz, 0, m) -
              PmC(2) * nuc3(Cart::zz, 0, m + 1) +
              2 * fak * (nuc3(Cart::z, 0, m) - nuc3(Cart::z, 0, m + 1));
        }
      }
      //------------------------------------------------------

      // Integrals     g - s
      if (lmax_row > 3) {
        for (Index m = 0; m < lsum - 3; m++) {
          double term_xx =
              fak * (nuc3(Cart::xx, 0, m) - nuc3(Cart::xx, 0, m + 1));
          double term_yy =
              fak * (nuc3(Cart::yy, 0, m) - nuc3(Cart::yy, 0, m + 1));
          double term_zz =
              fak * (nuc3(Cart::zz, 0, m) - nuc3(Cart::zz, 0, m + 1));
          nuc3(Cart::xxxx, 0, m) = PmA(0) * nuc3(Cart::xxx, 0, m) -
                                   PmC(0) * nuc3(Cart::xxx, 0, m + 1) +
                                   3 * term_xx;
          nuc3(Cart::xxxy, 0, m) = PmA(1) * nuc3(Cart::xxx, 0, m) -
                                   PmC(1) * nuc3(Cart::xxx, 0, m + 1);
          nuc3(Cart::xxxz, 0, m) = PmA(2) * nuc3(Cart::xxx, 0, m) -
                                   PmC(2) * nuc3(Cart::xxx, 0, m + 1);
          nuc3(Cart::xxyy, 0, m) = PmA(0) * nuc3(Cart::xyy, 0, m) -
                                   PmC(0) * nuc3(Cart::xyy, 0, m + 1) + term_yy;
          nuc3(Cart::xxyz, 0, m) = PmA(1) * nuc3(Cart::xxz, 0, m) -
                                   PmC(1) * nuc3(Cart::xxz, 0, m + 1);
          nuc3(Cart::xxzz, 0, m) = PmA(0) * nuc3(Cart::xzz, 0, m) -
                                   PmC(0) * nuc3(Cart::xzz, 0, m + 1) + term_zz;
          nuc3(Cart::xyyy, 0, m) = PmA(0) * nuc3(Cart::yyy, 0, m) -
                                   PmC(0) * nuc3(Cart::yyy, 0, m + 1);
          nuc3(Cart::xyyz, 0, m) = PmA(0) * nuc3(Cart::yyz, 0, m) -
                                   PmC(0) * nuc3(Cart::yyz, 0, m + 1);
          nuc3(Cart::xyzz, 0, m) = PmA(0) * nuc3(Cart::yzz, 0, m) -
                                   PmC(0) * nuc3(Cart::yzz, 0, m + 1);
          nuc3(Cart::xzzz, 0, m) = PmA(0) * nuc3(Cart::zzz, 0, m) -
                                   PmC(0) * nuc3(Cart::zzz, 0, m + 1);
          nuc3(Cart::yyyy, 0, m) = PmA(1) * nuc3(Cart::yyy, 0, m) -
                                   PmC(1) * nuc3(Cart::yyy, 0, m + 1) +
                                   3 * term_yy;
          nuc3(Cart::yyyz, 0, m) = PmA(2) * nuc3(Cart::yyy, 0, m) -
                                   PmC(2) * nuc3(Cart::yyy, 0, m + 1);
          nuc3(Cart::yyzz, 0, m) = PmA(1) * nuc3(Cart::yzz, 0, m) -
                                   PmC(1) * nuc3(Cart::yzz, 0, m + 1) + term_zz;
          nuc3(Cart::yzzz, 0, m) = PmA(1) * nuc3(Cart::zzz, 0, m) -
                                   PmC(1) * nuc3(Cart::zzz, 0, m + 1);
          nuc3(Cart::zzzz, 0, m) = PmA(2) * nuc3(Cart::zzz, 0, m) -
                                   PmC(2) * nuc3(Cart::zzz, 0, m + 1) +
                                   3 * term_zz;
        }
      }
      //------------------------------------------------------

      if (lmax_col > 0) {

        // Integrals     s - p
        for (Index m = 0; m < lmax_col; m++) {
          nuc3(0, Cart::x, m) =
              PmB(0) * nuc3(0, 0, m) - PmC(0) * nuc3(0, 0, m + 1);
          nuc3(0, Cart::y, m) =
              PmB(1) * nuc3(0, 0, m) - PmC(1) * nuc3(0, 0, m + 1);
          nuc3(0, Cart::z, m) =
              PmB(2) * nuc3(0, 0, m) - PmC(2) * nuc3(0, 0, m + 1);
        }
        //------------------------------------------------------

        // Integrals     p - p
        if (lmax_row > 0) {
          for (Index m = 0; m < lmax_col; m++) {
            double term = fak * (nuc3(0, 0, m) - nuc3(0, 0, m + 1));
            for (Index i = 1; i < 4; i++) {
              nuc3(i, Cart::x, m) = PmB(0) * nuc3(i, 0, m) -
                                    PmC(0) * nuc3(i, 0, m + 1) + nx[i] * term;
              nuc3(i, Cart::y, m) = PmB(1) * nuc3(i, 0, m) -
                                    PmC(1) * nuc3(i, 0, m + 1) + ny[i] * term;
              nuc3(i, Cart::z, m) = PmB(2) * nuc3(i, 0, m) -
                                    PmC(2) * nuc3(i, 0, m + 1) + nz[i] * term;
            }
          }
        }
        //------------------------------------------------------

        // Integrals     d - p     f - p     g - p
        for (Index m = 0; m < lmax_col; m++) {
          for (Index i = 4; i < n_orbitals[lmax_row]; i++) {
            int nx_i = nx[i];
            int ny_i = ny[i];
            int nz_i = nz[i];
            int ilx_i = i_less_x[i];
            int ily_i = i_less_y[i];
            int ilz_i = i_less_z[i];
            nuc3(i, Cart::x, m) =
                PmB(0) * nuc3(i, 0, m) - PmC(0) * nuc3(i, 0, m + 1) +
                nx_i * fak * (nuc3(ilx_i, 0, m) - nuc3(ilx_i, 0, m + 1));
            nuc3(i, Cart::y, m) =
                PmB(1) * nuc3(i, 0, m) - PmC(1) * nuc3(i, 0, m + 1) +
                ny_i * fak * (nuc3(ily_i, 0, m) - nuc3(ily_i, 0, m + 1));
            nuc3(i, Cart::z, m) =
                PmB(2) * nuc3(i, 0, m) - PmC(2) * nuc3(i, 0, m + 1) +
                nz_i * fak * (nuc3(ilz_i, 0, m) - nuc3(ilz_i, 0, m + 1));
          }
        }
        //------------------------------------------------------

      }  // end if (lmax_col > 0)

      if (lmax_col > 1) {

        // Integrals     s - d
        for (Index m = 0; m < lmax_col - 1; m++) {
          double term = fak * (nuc3(0, 0, m) - nuc3(0, 0, m + 1));
          nuc3(0, Cart::xx, m) = PmB(0) * nuc3(0, Cart::x, m) -
                                 PmC(0) * nuc3(0, Cart::x, m + 1) + term;
          nuc3(0, Cart::xy, m) =
              PmB(0) * nuc3(0, Cart::y, m) - PmC(0) * nuc3(0, Cart::y, m + 1);
          nuc3(0, Cart::xz, m) =
              PmB(0) * nuc3(0, Cart::z, m) - PmC(0) * nuc3(0, Cart::z, m + 1);
          nuc3(0, Cart::yy, m) = PmB(1) * nuc3(0, Cart::y, m) -
                                 PmC(1) * nuc3(0, Cart::y, m + 1) + term;
          nuc3(0, Cart::yz, m) =
              PmB(1) * nuc3(0, Cart::z, m) - PmC(1) * nuc3(0, Cart::z, m + 1);
          nuc3(0, Cart::zz, m) = PmB(2) * nuc3(0, Cart::z, m) -
                                 PmC(2) * nuc3(0, Cart::z, m + 1) + term;
        }
        //------------------------------------------------------

        // Integrals     p - d     d - d     f - d     g - d
        for (Index m = 0; m < lmax_col - 1; m++) {
          for (Index i = 1; i < n_orbitals[lmax_row]; i++) {
            int nx_i = nx[i];
            int ny_i = ny[i];
            int nz_i = nz[i];
            int ilx_i = i_less_x[i];
            int ily_i = i_less_y[i];
            int ilz_i = i_less_z[i];
            double term = fak * (nuc3(i, 0, m) - nuc3(i, 0, m + 1));
            nuc3(i, Cart::xx, m) =
                PmB(0) * nuc3(i, Cart::x, m) -
                PmC(0) * nuc3(i, Cart::x, m + 1) +
                nx_i * fak *
                    (nuc3(ilx_i, Cart::x, m) - nuc3(ilx_i, Cart::x, m + 1)) +
                term;
            nuc3(i, Cart::xy, m) =
                PmB(0) * nuc3(i, Cart::y, m) -
                PmC(0) * nuc3(i, Cart::y, m + 1) +
                nx_i * fak *
                    (nuc3(ilx_i, Cart::y, m) - nuc3(ilx_i, Cart::y, m + 1));
            nuc3(i, Cart::xz, m) =
                PmB(0) * nuc3(i, Cart::z, m) -
                PmC(0) * nuc3(i, Cart::z, m + 1) +
                nx_i * fak *
                    (nuc3(ilx_i, Cart::z, m) - nuc3(ilx_i, Cart::z, m + 1));
            nuc3(i, Cart::yy, m) =
                PmB(1) * nuc3(i, Cart::y, m) -
                PmC(1) * nuc3(i, Cart::y, m + 1) +
                ny_i * fak *
                    (nuc3(ily_i, Cart::y, m) - nuc3(ily_i, Cart::y, m + 1)) +
                term;
            nuc3(i, Cart::yz, m) =
                PmB(1) * nuc3(i, Cart::z, m) -
                PmC(1) * nuc3(i, Cart::z, m + 1) +
                ny_i * fak *
                    (nuc3(ily_i, Cart::z, m) - nuc3(ily_i, Cart::z, m + 1));
            nuc3(i, Cart::zz, m) =
                PmB(2) * nuc3(i, Cart::z, m) -
                PmC(2) * nuc3(i, Cart::z, m + 1) +
                nz_i * fak *
                    (nuc3(ilz_i, Cart::z, m) - nuc3(ilz_i, Cart::z, m + 1)) +
                term;
          }
        }
        //------------------------------------------------------

      }  // end if (lmax_col > 1)

      if (lmax_col > 2) {

        // Integrals     s - f
        for (Index m = 0; m < lmax_col - 2; m++) {
          nuc3(0, Cart::xxx, m) =
              PmB(0) * nuc3(0, Cart::xx, m) -
              PmC(0) * nuc3(0, Cart::xx, m + 1) +
              2 * fak * (nuc3(0, Cart::x, m) - nuc3(0, Cart::x, m + 1));
          nuc3(0, Cart::xxy, m) =
              PmB(1) * nuc3(0, Cart::xx, m) - PmC(1) * nuc3(0, Cart::xx, m + 1);
          nuc3(0, Cart::xxz, m) =
              PmB(2) * nuc3(0, Cart::xx, m) - PmC(2) * nuc3(0, Cart::xx, m + 1);
          nuc3(0, Cart::xyy, m) =
              PmB(0) * nuc3(0, Cart::yy, m) - PmC(0) * nuc3(0, Cart::yy, m + 1);
          nuc3(0, Cart::xyz, m) =
              PmB(0) * nuc3(0, Cart::yz, m) - PmC(0) * nuc3(0, Cart::yz, m + 1);
          nuc3(0, Cart::xzz, m) =
              PmB(0) * nuc3(0, Cart::zz, m) - PmC(0) * nuc3(0, Cart::zz, m + 1);
          nuc3(0, Cart::yyy, m) =
              PmB(1) * nuc3(0, Cart::yy, m) -
              PmC(1) * nuc3(0, Cart::yy, m + 1) +
              2 * fak * (nuc3(0, Cart::y, m) - nuc3(0, Cart::y, m + 1));
          nuc3(0, Cart::yyz, m) =
              PmB(2) * nuc3(0, Cart::yy, m) - PmC(2) * nuc3(0, Cart::yy, m + 1);
          nuc3(0, Cart::yzz, m) =
              PmB(1) * nuc3(0, Cart::zz, m) - PmC(1) * nuc3(0, Cart::zz, m + 1);
          nuc3(0, Cart::zzz, m) =
              PmB(2) * nuc3(0, Cart::zz, m) -
              PmC(2) * nuc3(0, Cart::zz, m + 1) +
              2 * fak * (nuc3(0, Cart::z, m) - nuc3(0, Cart::z, m + 1));
        }
        //------------------------------------------------------

        // Integrals     p - f     d - f     f - f     g - f
        for (Index m = 0; m < lmax_col - 2; m++) {
          for (Index i = 1; i < n_orbitals[lmax_row]; i++) {
            int nx_i = nx[i];
            int ny_i = ny[i];
            int nz_i = nz[i];
            int ilx_i = i_less_x[i];
            int ily_i = i_less_y[i];
            int ilz_i = i_less_z[i];
            double term_x =
                2 * fak * (nuc3(i, Cart::x, m) - nuc3(i, Cart::x, m + 1));
            double term_y =
                2 * fak * (nuc3(i, Cart::y, m) - nuc3(i, Cart::y, m + 1));
            double term_z =
                2 * fak * (nuc3(i, Cart::z, m) - nuc3(i, Cart::z, m + 1));
            nuc3(i, Cart::xxx, m) =
                PmB(0) * nuc3(i, Cart::xx, m) -
                PmC(0) * nuc3(i, Cart::xx, m + 1) +
                nx_i * fak *
                    (nuc3(ilx_i, Cart::xx, m) - nuc3(ilx_i, Cart::xx, m + 1)) +
                term_x;
            nuc3(i, Cart::xxy, m) =
                PmB(1) * nuc3(i, Cart::xx, m) -
                PmC(1) * nuc3(i, Cart::xx, m + 1) +
                ny_i * fak *
                    (nuc3(ily_i, Cart::xx, m) - nuc3(ily_i, Cart::xx, m + 1));
            nuc3(i, Cart::xxz, m) =
                PmB(2) * nuc3(i, Cart::xx, m) -
                PmC(2) * nuc3(i, Cart::xx, m + 1) +
                nz_i * fak *
                    (nuc3(ilz_i, Cart::xx, m) - nuc3(ilz_i, Cart::xx, m + 1));
            nuc3(i, Cart::xyy, m) =
                PmB(0) * nuc3(i, Cart::yy, m) -
                PmC(0) * nuc3(i, Cart::yy, m + 1) +
                nx_i * fak *
                    (nuc3(ilx_i, Cart::yy, m) - nuc3(ilx_i, Cart::yy, m + 1));
            nuc3(i, Cart::xyz, m) =
                PmB(0) * nuc3(i, Cart::yz, m) -
                PmC(0) * nuc3(i, Cart::yz, m + 1) +
                nx_i * fak *
                    (nuc3(ilx_i, Cart::yz, m) - nuc3(ilx_i, Cart::yz, m + 1));
            nuc3(i, Cart::xzz, m) =
                PmB(0) * nuc3(i, Cart::zz, m) -
                PmC(0) * nuc3(i, Cart::zz, m + 1) +
                nx_i * fak *
                    (nuc3(ilx_i, Cart::zz, m) - nuc3(ilx_i, Cart::zz, m + 1));
            nuc3(i, Cart::yyy, m) =
                PmB(1) * nuc3(i, Cart::yy, m) -
                PmC(1) * nuc3(i, Cart::yy, m + 1) +
                ny_i * fak *
                    (nuc3(ily_i, Cart::yy, m) - nuc3(ily_i, Cart::yy, m + 1)) +
                term_y;
            nuc3(i, Cart::yyz, m) =
                PmB(2) * nuc3(i, Cart::yy, m) -
                PmC(2) * nuc3(i, Cart::yy, m + 1) +
                nz_i * fak *
                    (nuc3(ilz_i, Cart::yy, m) - nuc3(ilz_i, Cart::yy, m + 1));
            nuc3(i, Cart::yzz, m) =
                PmB(1) * nuc3(i, Cart::zz, m) -
                PmC(1) * nuc3(i, Cart::zz, m + 1) +
                ny_i * fak *
                    (nuc3(ily_i, Cart::zz, m) - nuc3(ily_i, Cart::zz, m + 1));
            nuc3(i, Cart::zzz, m) =
                PmB(2) * nuc3(i, Cart::zz, m) -
                PmC(2) * nuc3(i, Cart::zz, m + 1) +
                nz_i * fak *
                    (nuc3(ilz_i, Cart::zz, m) - nuc3(ilz_i, Cart::zz, m + 1)) +
                term_z;
          }
        }
        //------------------------------------------------------

      }  // end if (lmax_col > 2)

      if (lmax_col > 3) {

        // Integrals     s - g
        for (Index m = 0; m < lmax_col - 3; m++) {
          double term_xx =
              fak * (nuc3(0, Cart::xx, m) - nuc3(0, Cart::xx, m + 1));
          double term_yy =
              fak * (nuc3(0, Cart::yy, m) - nuc3(0, Cart::yy, m + 1));
          double term_zz =
              fak * (nuc3(0, Cart::zz, m) - nuc3(0, Cart::zz, m + 1));
          nuc3(0, Cart::xxxx, m) = PmB(0) * nuc3(0, Cart::xxx, m) -
                                   PmC(0) * nuc3(0, Cart::xxx, m + 1) +
                                   3 * term_xx;
          nuc3(0, Cart::xxxy, m) = PmB(1) * nuc3(0, Cart::xxx, m) -
                                   PmC(1) * nuc3(0, Cart::xxx, m + 1);
          nuc3(0, Cart::xxxz, m) = PmB(2) * nuc3(0, Cart::xxx, m) -
                                   PmC(2) * nuc3(0, Cart::xxx, m + 1);
          nuc3(0, Cart::xxyy, m) = PmB(0) * nuc3(0, Cart::xyy, m) -
                                   PmC(0) * nuc3(0, Cart::xyy, m + 1) + term_yy;
          nuc3(0, Cart::xxyz, m) = PmB(1) * nuc3(0, Cart::xxz, m) -
                                   PmC(1) * nuc3(0, Cart::xxz, m + 1);
          nuc3(0, Cart::xxzz, m) = PmB(0) * nuc3(0, Cart::xzz, m) -
                                   PmC(0) * nuc3(0, Cart::xzz, m + 1) + term_zz;
          nuc3(0, Cart::xyyy, m) = PmB(0) * nuc3(0, Cart::yyy, m) -
                                   PmC(0) * nuc3(0, Cart::yyy, m + 1);
          nuc3(0, Cart::xyyz, m) = PmB(0) * nuc3(0, Cart::yyz, m) -
                                   PmC(0) * nuc3(0, Cart::yyz, m + 1);
          nuc3(0, Cart::xyzz, m) = PmB(0) * nuc3(0, Cart::yzz, m) -
                                   PmC(0) * nuc3(0, Cart::yzz, m + 1);
          nuc3(0, Cart::xzzz, m) = PmB(0) * nuc3(0, Cart::zzz, m) -
                                   PmC(0) * nuc3(0, Cart::zzz, m + 1);
          nuc3(0, Cart::yyyy, m) = PmB(1) * nuc3(0, Cart::yyy, m) -
                                   PmC(1) * nuc3(0, Cart::yyy, m + 1) +
                                   3 * term_yy;
          nuc3(0, Cart::yyyz, m) = PmB(2) * nuc3(0, Cart::yyy, m) -
                                   PmC(2) * nuc3(0, Cart::yyy, m + 1);
          nuc3(0, Cart::yyzz, m) = PmB(1) * nuc3(0, Cart::yzz, m) -
                                   PmC(1) * nuc3(0, Cart::yzz, m + 1) + term_zz;
          nuc3(0, Cart::yzzz, m) = PmB(1) * nuc3(0, Cart::zzz, m) -
                                   PmC(1) * nuc3(0, Cart::zzz, m + 1);
          nuc3(0, Cart::zzzz, m) = PmB(2) * nuc3(0, Cart::zzz, m) -
                                   PmC(2) * nuc3(0, Cart::zzz, m + 1) +
                                   3 * term_zz;
        }
        //------------------------------------------------------

        // Integrals     p - g     d - g     f - g     g - g
        for (Index m = 0; m < lmax_col - 3; m++) {
          for (Index i = 1; i < n_orbitals[lmax_row]; i++) {
            int nx_i = nx[i];
            int ny_i = ny[i];
            int nz_i = nz[i];
            int ilx_i = i_less_x[i];
            int ily_i = i_less_y[i];
            int ilz_i = i_less_z[i];
            double term_xx =
                fak * (nuc3(i, Cart::xx, m) - nuc3(i, Cart::xx, m + 1));
            double term_yy =
                fak * (nuc3(i, Cart::yy, m) - nuc3(i, Cart::yy, m + 1));
            double term_zz =
                fak * (nuc3(i, Cart::zz, m) - nuc3(i, Cart::zz, m + 1));
            nuc3(i, Cart::xxxx, m) = PmB(0) * nuc3(i, Cart::xxx, m) -
                                     PmC(0) * nuc3(i, Cart::xxx, m + 1) +
                                     nx_i * fak *
                                         (nuc3(ilx_i, Cart::xxx, m) -
                                          nuc3(ilx_i, Cart::xxx, m + 1)) +
                                     3 * term_xx;
            nuc3(i, Cart::xxxy, m) =
                PmB(1) * nuc3(i, Cart::xxx, m) -
                PmC(1) * nuc3(i, Cart::xxx, m + 1) +
                ny_i * fak *
                    (nuc3(ily_i, Cart::xxx, m) - nuc3(ily_i, Cart::xxx, m + 1));
            nuc3(i, Cart::xxxz, m) =
                PmB(2) * nuc3(i, Cart::xxx, m) -
                PmC(2) * nuc3(i, Cart::xxx, m + 1) +
                nz_i * fak *
                    (nuc3(ilz_i, Cart::xxx, m) - nuc3(ilz_i, Cart::xxx, m + 1));
            nuc3(i, Cart::xxyy, m) = PmB(0) * nuc3(i, Cart::xyy, m) -
                                     PmC(0) * nuc3(i, Cart::xyy, m + 1) +
                                     nx_i * fak *
                                         (nuc3(ilx_i, Cart::xyy, m) -
                                          nuc3(ilx_i, Cart::xyy, m + 1)) +
                                     term_yy;
            nuc3(i, Cart::xxyz, m) =
                PmB(1) * nuc3(i, Cart::xxz, m) -
                PmC(1) * nuc3(i, Cart::xxz, m + 1) +
                ny_i * fak *
                    (nuc3(ily_i, Cart::xxz, m) - nuc3(ily_i, Cart::xxz, m + 1));
            nuc3(i, Cart::xxzz, m) = PmB(0) * nuc3(i, Cart::xzz, m) -
                                     PmC(0) * nuc3(i, Cart::xzz, m + 1) +
                                     nx_i * fak *
                                         (nuc3(ilx_i, Cart::xzz, m) -
                                          nuc3(ilx_i, Cart::xzz, m + 1)) +
                                     term_zz;
            nuc3(i, Cart::xyyy, m) =
                PmB(0) * nuc3(i, Cart::yyy, m) -
                PmC(0) * nuc3(i, Cart::yyy, m + 1) +
                nx_i * fak *
                    (nuc3(ilx_i, Cart::yyy, m) - nuc3(ilx_i, Cart::yyy, m + 1));
            nuc3(i, Cart::xyyz, m) =
                PmB(0) * nuc3(i, Cart::yyz, m) -
                PmC(0) * nuc3(i, Cart::yyz, m + 1) +
                nx_i * fak *
                    (nuc3(ilx_i, Cart::yyz, m) - nuc3(ilx_i, Cart::yyz, m + 1));
            nuc3(i, Cart::xyzz, m) =
                PmB(0) * nuc3(i, Cart::yzz, m) -
                PmC(0) * nuc3(i, Cart::yzz, m + 1) +
                nx_i * fak *
                    (nuc3(ilx_i, Cart::yzz, m) - nuc3(ilx_i, Cart::yzz, m + 1));
            nuc3(i, Cart::xzzz, m) =
                PmB(0) * nuc3(i, Cart::zzz, m) -
                PmC(0) * nuc3(i, Cart::zzz, m + 1) +
                nx_i * fak *
                    (nuc3(ilx_i, Cart::zzz, m) - nuc3(ilx_i, Cart::zzz, m + 1));
            nuc3(i, Cart::yyyy, m) = PmB(1) * nuc3(i, Cart::yyy, m) -
                                     PmC(1) * nuc3(i, Cart::yyy, m + 1) +
                                     ny_i * fak *
                                         (nuc3(ily_i, Cart::yyy, m) -
                                          nuc3(ily_i, Cart::yyy, m + 1)) +
                                     3 * term_yy;
            nuc3(i, Cart::yyyz, m) =
                PmB(2) * nuc3(i, Cart::yyy, m) -
                PmC(2) * nuc3(i, Cart::yyy, m + 1) +
                nz_i * fak *
                    (nuc3(ilz_i, Cart::yyy, m) - nuc3(ilz_i, Cart::yyy, m + 1));
            nuc3(i, Cart::yyzz, m) = PmB(1) * nuc3(i, Cart::yzz, m) -
                                     PmC(1) * nuc3(i, Cart::yzz, m + 1) +
                                     ny_i * fak *
                                         (nuc3(ily_i, Cart::yzz, m) -
                                          nuc3(ily_i, Cart::yzz, m + 1)) +
                                     term_zz;
            nuc3(i, Cart::yzzz, m) =
                PmB(1) * nuc3(i, Cart::zzz, m) -
                PmC(1) * nuc3(i, Cart::zzz, m + 1) +
                ny_i * fak *
                    (nuc3(ily_i, Cart::zzz, m) - nuc3(ily_i, Cart::zzz, m + 1));
            nuc3(i, Cart::zzzz, m) = PmB(2) * nuc3(i, Cart::zzz, m) -
                                     PmC(2) * nuc3(i, Cart::zzz, m + 1) +
                                     nz_i * fak *
                                         (nuc3(ilz_i, Cart::zzz, m) -
                                          nuc3(ilz_i, Cart::zzz, m + 1)) +
                                     3 * term_zz;
          }
        }
        //------------------------------------------------------
      }  // end if (lmax_col > 3)

      if (rank > 0) {
        Eigen::Tensor<double, 4> dip4(nrows, ncols, 3, lsum + 1);
        dip4.setZero();

        // (s-s element normiert )
        double _prefactor_dip = 2. * zeta * prefactor;
        for (Index m = 0; m < lsum + 1; m++) {
          dip4(0, 0, 0, m) = PmC(0) * _prefactor_dip * FmU[m + 1];
          dip4(0, 0, 1, m) = PmC(1) * _prefactor_dip * FmU[m + 1];
          dip4(0, 0, 2, m) = PmC(2) * _prefactor_dip * FmU[m + 1];
        }
        //------------------------------------------------------

        // Integrals     p - s
        if (lmax_row > 0) {
          for (Index m = 0; m < lsum; m++) {
            for (Index k = 0; k < 3; k++) {
              dip4(Cart::x, 0, k, m) = PmA(0) * dip4(0, 0, k, m) -
                                       PmC(0) * dip4(0, 0, k, m + 1) +
                                       (k == 0) * nuc3(0, 0, m + 1);
              dip4(Cart::y, 0, k, m) = PmA(1) * dip4(0, 0, k, m) -
                                       PmC(1) * dip4(0, 0, k, m + 1) +
                                       (k == 1) * nuc3(0, 0, m + 1);
              dip4(Cart::z, 0, k, m) = PmA(2) * dip4(0, 0, k, m) -
                                       PmC(2) * dip4(0, 0, k, m + 1) +
                                       (k == 2) * nuc3(0, 0, m + 1);
            }
          }
        }
        //------------------------------------------------------

        // Integrals     d - s
        if (lmax_row > 1) {
          for (Index m = 0; m < lsum - 1; m++) {
            for (Index k = 0; k < 3; k++) {
              double term = fak * (dip4(0, 0, k, m) - dip4(0, 0, k, m + 1));
              dip4(Cart::xx, 0, k, m) = PmA(0) * dip4(Cart::x, 0, k, m) -
                                        PmC(0) * dip4(Cart::x, 0, k, m + 1) +
                                        (k == 0) * nuc3(Cart::x, 0, m + 1) +
                                        term;
              dip4(Cart::xy, 0, k, m) = PmA(0) * dip4(Cart::y, 0, k, m) -
                                        PmC(0) * dip4(Cart::y, 0, k, m + 1) +
                                        (k == 0) * nuc3(Cart::y, 0, m + 1);
              dip4(Cart::xz, 0, k, m) = PmA(0) * dip4(Cart::z, 0, k, m) -
                                        PmC(0) * dip4(Cart::z, 0, k, m + 1) +
                                        (k == 0) * nuc3(Cart::z, 0, m + 1);
              dip4(Cart::yy, 0, k, m) = PmA(1) * dip4(Cart::y, 0, k, m) -
                                        PmC(1) * dip4(Cart::y, 0, k, m + 1) +
                                        (k == 1) * nuc3(Cart::y, 0, m + 1) +
                                        term;
              dip4(Cart::yz, 0, k, m) = PmA(1) * dip4(Cart::z, 0, k, m) -
                                        PmC(1) * dip4(Cart::z, 0, k, m + 1) +
                                        (k == 1) * nuc3(Cart::z, 0, m + 1);
              dip4(Cart::zz, 0, k, m) = PmA(2) * dip4(Cart::z, 0, k, m) -
                                        PmC(2) * dip4(Cart::z, 0, k, m + 1) +
                                        (k == 2) * nuc3(Cart::z, 0, m + 1) +
                                        term;
            }
          }
        }
        //------------------------------------------------------

        // Integrals     f - s
        if (lmax_row > 2) {
          for (Index m = 0; m < lsum - 2; m++) {
            for (Index k = 0; k < 3; k++) {
              dip4(Cart::xxx, 0, k, m) =
                  PmA(0) * dip4(Cart::xx, 0, k, m) -
                  PmC(0) * dip4(Cart::xx, 0, k, m + 1) +
                  (k == 0) * nuc3(Cart::xx, 0, m + 1) +
                  2 * fak *
                      (dip4(Cart::x, 0, k, m) - dip4(Cart::x, 0, k, m + 1));
              dip4(Cart::xxy, 0, k, m) = PmA(1) * dip4(Cart::xx, 0, k, m) -
                                         PmC(1) * dip4(Cart::xx, 0, k, m + 1) +
                                         (k == 1) * nuc3(Cart::xx, 0, m + 1);
              dip4(Cart::xxz, 0, k, m) = PmA(2) * dip4(Cart::xx, 0, k, m) -
                                         PmC(2) * dip4(Cart::xx, 0, k, m + 1) +
                                         (k == 2) * nuc3(Cart::xx, 0, m + 1);
              dip4(Cart::xyy, 0, k, m) = PmA(0) * dip4(Cart::yy, 0, k, m) -
                                         PmC(0) * dip4(Cart::yy, 0, k, m + 1) +
                                         (k == 0) * nuc3(Cart::yy, 0, m + 1);
              dip4(Cart::xyz, 0, k, m) = PmA(0) * dip4(Cart::yz, 0, k, m) -
                                         PmC(0) * dip4(Cart::yz, 0, k, m + 1) +
                                         (k == 0) * nuc3(Cart::yz, 0, m + 1);
              dip4(Cart::xzz, 0, k, m) = PmA(0) * dip4(Cart::zz, 0, k, m) -
                                         PmC(0) * dip4(Cart::zz, 0, k, m + 1) +
                                         (k == 0) * nuc3(Cart::zz, 0, m + 1);
              dip4(Cart::yyy, 0, k, m) =
                  PmA(1) * dip4(Cart::yy, 0, k, m) -
                  PmC(1) * dip4(Cart::yy, 0, k, m + 1) +
                  (k == 1) * nuc3(Cart::yy, 0, m + 1) +
                  2 * fak *
                      (dip4(Cart::y, 0, k, m) - dip4(Cart::y, 0, k, m + 1));
              dip4(Cart::yyz, 0, k, m) = PmA(2) * dip4(Cart::yy, 0, k, m) -
                                         PmC(2) * dip4(Cart::yy, 0, k, m + 1) +
                                         (k == 2) * nuc3(Cart::yy, 0, m + 1);
              dip4(Cart::yzz, 0, k, m) = PmA(1) * dip4(Cart::zz, 0, k, m) -
                                         PmC(1) * dip4(Cart::zz, 0, k, m + 1) +
                                         (k == 1) * nuc3(Cart::zz, 0, m + 1);
              dip4(Cart::zzz, 0, k, m) =
                  PmA(2) * dip4(Cart::zz, 0, k, m) -
                  PmC(2) * dip4(Cart::zz, 0, k, m + 1) +
                  (k == 2) * nuc3(Cart::zz, 0, m + 1) +
                  2 * fak *
                      (dip4(Cart::z, 0, k, m) - dip4(Cart::z, 0, k, m + 1));
            }
          }
        }
        //------------------------------------------------------

        // Integrals     g - s
        if (lmax_row > 3) {
          for (Index m = 0; m < lsum - 3; m++) {
            for (Index k = 0; k < 3; k++) {
              double term_xx =
                  fak * (dip4(Cart::xx, 0, k, m) - dip4(Cart::xx, 0, k, m + 1));
              double term_yy =
                  fak * (dip4(Cart::yy, 0, k, m) - dip4(Cart::yy, 0, k, m + 1));
              double term_zz =
                  fak * (dip4(Cart::zz, 0, k, m) - dip4(Cart::zz, 0, k, m + 1));
              dip4(Cart::xxxx, 0, k, m) =
                  PmA(0) * dip4(Cart::xxx, 0, k, m) -
                  PmC(0) * dip4(Cart::xxx, 0, k, m + 1) +
                  (k == 0) * nuc3(Cart::xxx, 0, m + 1) + 3 * term_xx;
              dip4(Cart::xxxy, 0, k, m) =
                  PmA(1) * dip4(Cart::xxx, 0, k, m) -
                  PmC(1) * dip4(Cart::xxx, 0, k, m + 1) +
                  (k == 1) * nuc3(Cart::xxx, 0, m + 1);
              dip4(Cart::xxxz, 0, k, m) =
                  PmA(2) * dip4(Cart::xxx, 0, k, m) -
                  PmC(2) * dip4(Cart::xxx, 0, k, m + 1) +
                  (k == 2) * nuc3(Cart::xxx, 0, m + 1);
              dip4(Cart::xxyy, 0, k, m) =
                  PmA(0) * dip4(Cart::xyy, 0, k, m) -
                  PmC(0) * dip4(Cart::xyy, 0, k, m + 1) +
                  (k == 0) * nuc3(Cart::xyy, 0, m + 1) + term_yy;
              dip4(Cart::xxyz, 0, k, m) =
                  PmA(1) * dip4(Cart::xxz, 0, k, m) -
                  PmC(1) * dip4(Cart::xxz, 0, k, m + 1) +
                  (k == 1) * nuc3(Cart::xxz, 0, m + 1);
              dip4(Cart::xxzz, 0, k, m) =
                  PmA(0) * dip4(Cart::xzz, 0, k, m) -
                  PmC(0) * dip4(Cart::xzz, 0, k, m + 1) +
                  (k == 0) * nuc3(Cart::xzz, 0, m + 1) + term_zz;
              dip4(Cart::xyyy, 0, k, m) =
                  PmA(0) * dip4(Cart::yyy, 0, k, m) -
                  PmC(0) * dip4(Cart::yyy, 0, k, m + 1) +
                  (k == 0) * nuc3(Cart::yyy, 0, m + 1);
              dip4(Cart::xyyz, 0, k, m) =
                  PmA(0) * dip4(Cart::yyz, 0, k, m) -
                  PmC(0) * dip4(Cart::yyz, 0, k, m + 1) +
                  (k == 0) * nuc3(Cart::yyz, 0, m + 1);
              dip4(Cart::xyzz, 0, k, m) =
                  PmA(0) * dip4(Cart::yzz, 0, k, m) -
                  PmC(0) * dip4(Cart::yzz, 0, k, m + 1) +
                  (k == 0) * nuc3(Cart::yzz, 0, m + 1);
              dip4(Cart::xzzz, 0, k, m) =
                  PmA(0) * dip4(Cart::zzz, 0, k, m) -
                  PmC(0) * dip4(Cart::zzz, 0, k, m + 1) +
                  (k == 0) * nuc3(Cart::zzz, 0, m + 1);
              dip4(Cart::yyyy, 0, k, m) =
                  PmA(1) * dip4(Cart::yyy, 0, k, m) -
                  PmC(1) * dip4(Cart::yyy, 0, k, m + 1) +
                  (k == 1) * nuc3(Cart::yyy, 0, m + 1) + 3 * term_yy;
              dip4(Cart::yyyz, 0, k, m) =
                  PmA(2) * dip4(Cart::yyy, 0, k, m) -
                  PmC(2) * dip4(Cart::yyy, 0, k, m + 1) +
                  (k == 2) * nuc3(Cart::yyy, 0, m + 1);
              dip4(Cart::yyzz, 0, k, m) =
                  PmA(1) * dip4(Cart::yzz, 0, k, m) -
                  PmC(1) * dip4(Cart::yzz, 0, k, m + 1) +
                  (k == 1) * nuc3(Cart::yzz, 0, m + 1) + term_zz;
              dip4(Cart::yzzz, 0, k, m) =
                  PmA(1) * dip4(Cart::zzz, 0, k, m) -
                  PmC(1) * dip4(Cart::zzz, 0, k, m + 1) +
                  (k == 1) * nuc3(Cart::zzz, 0, m + 1);
              dip4(Cart::zzzz, 0, k, m) =
                  PmA(2) * dip4(Cart::zzz, 0, k, m) -
                  PmC(2) * dip4(Cart::zzz, 0, k, m + 1) +
                  (k == 2) * nuc3(Cart::zzz, 0, m + 1) + 3 * term_zz;
            }
          }
        }
        //------------------------------------------------------

        if (lmax_col > 0) {

          // Integrals     s - p
          for (Index m = 0; m < lmax_col; m++) {
            for (Index k = 0; k < 3; k++) {
              dip4(0, Cart::x, k, m) = PmB(0) * dip4(0, 0, k, m) -
                                       PmC(0) * dip4(0, 0, k, m + 1) +
                                       (k == 0) * nuc3(0, 0, m + 1);
              dip4(0, Cart::y, k, m) = PmB(1) * dip4(0, 0, k, m) -
                                       PmC(1) * dip4(0, 0, k, m + 1) +
                                       (k == 1) * nuc3(0, 0, m + 1);
              dip4(0, Cart::z, k, m) = PmB(2) * dip4(0, 0, k, m) -
                                       PmC(2) * dip4(0, 0, k, m + 1) +
                                       (k == 2) * nuc3(0, 0, m + 1);
            }
          }
          //------------------------------------------------------

          // Integrals     p - p
          if (lmax_row > 0) {
            for (Index m = 0; m < lmax_col; m++) {
              for (Index i = 1; i < 4; i++) {
                for (Index k = 0; k < 3; k++) {
                  double term = fak * (dip4(0, 0, k, m) - dip4(0, 0, k, m + 1));
                  dip4(i, Cart::x, k, m) = PmB(0) * dip4(i, 0, k, m) -
                                           PmC(0) * dip4(i, 0, k, m + 1) +
                                           (k == 0) * nuc3(i, 0, m + 1) +
                                           nx[i] * term;
                  dip4(i, Cart::y, k, m) = PmB(1) * dip4(i, 0, k, m) -
                                           PmC(1) * dip4(i, 0, k, m + 1) +
                                           (k == 1) * nuc3(i, 0, m + 1) +
                                           ny[i] * term;
                  dip4(i, Cart::z, k, m) = PmB(2) * dip4(i, 0, k, m) -
                                           PmC(2) * dip4(i, 0, k, m + 1) +
                                           (k == 2) * nuc3(i, 0, m + 1) +
                                           nz[i] * term;
                }
              }
            }
          }
          //------------------------------------------------------

          // Integrals     d - p     f - p     g - p
          for (Index m = 0; m < lmax_col; m++) {
            for (Index i = 4; i < n_orbitals[lmax_row]; i++) {
              int nx_i = nx[i];
              int ny_i = ny[i];
              int nz_i = nz[i];
              int ilx_i = i_less_x[i];
              int ily_i = i_less_y[i];
              int ilz_i = i_less_z[i];
              for (Index k = 0; k < 3; k++) {
                dip4(i, Cart::x, k, m) =
                    PmB(0) * dip4(i, 0, k, m) - PmC(0) * dip4(i, 0, k, m + 1) +
                    (k == 0) * nuc3(i, 0, m + 1) +
                    nx_i * fak *
                        (dip4(ilx_i, 0, k, m) - dip4(ilx_i, 0, k, m + 1));
                dip4(i, Cart::y, k, m) =
                    PmB(1) * dip4(i, 0, k, m) - PmC(1) * dip4(i, 0, k, m + 1) +
                    (k == 1) * nuc3(i, 0, m + 1) +
                    ny_i * fak *
                        (dip4(ily_i, 0, k, m) - dip4(ily_i, 0, k, m + 1));
                dip4(i, Cart::z, k, m) =
                    PmB(2) * dip4(i, 0, k, m) - PmC(2) * dip4(i, 0, k, m + 1) +
                    (k == 2) * nuc3(i, 0, m + 1) +
                    nz_i * fak *
                        (dip4(ilz_i, 0, k, m) - dip4(ilz_i, 0, k, m + 1));
              }
            }
          }
          //------------------------------------------------------

        }  // end if (lmax_col > 0)

        if (lmax_col > 1) {

          // Integrals     s - d
          for (Index m = 0; m < lmax_col - 1; m++) {
            for (Index k = 0; k < 3; k++) {
              double term = fak * (dip4(0, 0, k, m) - dip4(0, 0, k, m + 1));
              dip4(0, Cart::xx, k, m) = PmB(0) * dip4(0, Cart::x, k, m) -
                                        PmC(0) * dip4(0, Cart::x, k, m + 1) +
                                        (k == 0) * nuc3(0, Cart::x, m + 1) +
                                        term;
              dip4(0, Cart::xy, k, m) = PmB(0) * dip4(0, Cart::y, k, m) -
                                        PmC(0) * dip4(0, Cart::y, k, m + 1) +
                                        (k == 0) * nuc3(0, Cart::y, m + 1);
              dip4(0, Cart::xz, k, m) = PmB(0) * dip4(0, Cart::z, k, m) -
                                        PmC(0) * dip4(0, Cart::z, k, m + 1) +
                                        (k == 0) * nuc3(0, Cart::z, m + 1);
              dip4(0, Cart::yy, k, m) = PmB(1) * dip4(0, Cart::y, k, m) -
                                        PmC(1) * dip4(0, Cart::y, k, m + 1) +
                                        (k == 1) * nuc3(0, Cart::y, m + 1) +
                                        term;
              dip4(0, Cart::yz, k, m) = PmB(1) * dip4(0, Cart::z, k, m) -
                                        PmC(1) * dip4(0, Cart::z, k, m + 1) +
                                        (k == 1) * nuc3(0, Cart::z, m + 1);
              dip4(0, Cart::zz, k, m) = PmB(2) * dip4(0, Cart::z, k, m) -
                                        PmC(2) * dip4(0, Cart::z, k, m + 1) +
                                        (k == 2) * nuc3(0, Cart::z, m + 1) +
                                        term;
            }
          }
          //------------------------------------------------------

          // Integrals     p - d     d - d     f - d     g - d
          for (Index m = 0; m < lmax_col - 1; m++) {
            for (Index i = 1; i < n_orbitals[lmax_row]; i++) {
              int nx_i = nx[i];
              int ny_i = ny[i];
              int nz_i = nz[i];
              int ilx_i = i_less_x[i];
              int ily_i = i_less_y[i];
              int ilz_i = i_less_z[i];
              for (Index k = 0; k < 3; k++) {
                double term = fak * (dip4(i, 0, k, m) - dip4(i, 0, k, m + 1));
                dip4(i, Cart::xx, k, m) = PmB(0) * dip4(i, Cart::x, k, m) -
                                          PmC(0) * dip4(i, Cart::x, k, m + 1) +
                                          (k == 0) * nuc3(i, Cart::x, m + 1) +
                                          nx_i * fak *
                                              (dip4(ilx_i, Cart::x, k, m) -
                                               dip4(ilx_i, Cart::x, k, m + 1)) +
                                          term;
                dip4(i, Cart::xy, k, m) = PmB(0) * dip4(i, Cart::y, k, m) -
                                          PmC(0) * dip4(i, Cart::y, k, m + 1) +
                                          (k == 0) * nuc3(i, Cart::y, m + 1) +
                                          nx_i * fak *
                                              (dip4(ilx_i, Cart::y, k, m) -
                                               dip4(ilx_i, Cart::y, k, m + 1));
                dip4(i, Cart::xz, k, m) = PmB(0) * dip4(i, Cart::z, k, m) -
                                          PmC(0) * dip4(i, Cart::z, k, m + 1) +
                                          (k == 0) * nuc3(i, Cart::z, m + 1) +
                                          nx_i * fak *
                                              (dip4(ilx_i, Cart::z, k, m) -
                                               dip4(ilx_i, Cart::z, k, m + 1));
                dip4(i, Cart::yy, k, m) = PmB(1) * dip4(i, Cart::y, k, m) -
                                          PmC(1) * dip4(i, Cart::y, k, m + 1) +
                                          (k == 1) * nuc3(i, Cart::y, m + 1) +
                                          ny_i * fak *
                                              (dip4(ily_i, Cart::y, k, m) -
                                               dip4(ily_i, Cart::y, k, m + 1)) +
                                          term;
                dip4(i, Cart::yz, k, m) = PmB(1) * dip4(i, Cart::z, k, m) -
                                          PmC(1) * dip4(i, Cart::z, k, m + 1) +
                                          (k == 1) * nuc3(i, Cart::z, m + 1) +
                                          ny_i * fak *
                                              (dip4(ily_i, Cart::z, k, m) -
                                               dip4(ily_i, Cart::z, k, m + 1));
                dip4(i, Cart::zz, k, m) = PmB(2) * dip4(i, Cart::z, k, m) -
                                          PmC(2) * dip4(i, Cart::z, k, m + 1) +
                                          (k == 2) * nuc3(i, Cart::z, m + 1) +
                                          nz_i * fak *
                                              (dip4(ilz_i, Cart::z, k, m) -
                                               dip4(ilz_i, Cart::z, k, m + 1)) +
                                          term;
              }
            }
          }
          //------------------------------------------------------

        }  // end if (lmax_col > 1)

        if (lmax_col > 2) {

          // Integrals     s - f
          for (Index m = 0; m < lmax_col - 2; m++) {
            for (Index k = 0; k < 3; k++) {
              dip4(0, Cart::xxx, k, m) =
                  PmB(0) * dip4(0, Cart::xx, k, m) -
                  PmC(0) * dip4(0, Cart::xx, k, m + 1) +
                  (k == 0) * nuc3(0, Cart::xx, m + 1) +
                  2 * fak *
                      (dip4(0, Cart::x, k, m) - dip4(0, Cart::x, k, m + 1));
              dip4(0, Cart::xxy, k, m) = PmB(1) * dip4(0, Cart::xx, k, m) -
                                         PmC(1) * dip4(0, Cart::xx, k, m + 1) +
                                         (k == 1) * nuc3(0, Cart::xx, m + 1);
              dip4(0, Cart::xxz, k, m) = PmB(2) * dip4(0, Cart::xx, k, m) -
                                         PmC(2) * dip4(0, Cart::xx, k, m + 1) +
                                         (k == 2) * nuc3(0, Cart::xx, m + 1);
              dip4(0, Cart::xyy, k, m) = PmB(0) * dip4(0, Cart::yy, k, m) -
                                         PmC(0) * dip4(0, Cart::yy, k, m + 1) +
                                         (k == 0) * nuc3(0, Cart::yy, m + 1);
              dip4(0, Cart::xyz, k, m) = PmB(0) * dip4(0, Cart::yz, k, m) -
                                         PmC(0) * dip4(0, Cart::yz, k, m + 1) +
                                         (k == 0) * nuc3(0, Cart::yz, m + 1);
              dip4(0, Cart::xzz, k, m) = PmB(0) * dip4(0, Cart::zz, k, m) -
                                         PmC(0) * dip4(0, Cart::zz, k, m + 1) +
                                         (k == 0) * nuc3(0, Cart::zz, m + 1);
              dip4(0, Cart::yyy, k, m) =
                  PmB(1) * dip4(0, Cart::yy, k, m) -
                  PmC(1) * dip4(0, Cart::yy, k, m + 1) +
                  (k == 1) * nuc3(0, Cart::yy, m + 1) +
                  2 * fak *
                      (dip4(0, Cart::y, k, m) - dip4(0, Cart::y, k, m + 1));
              dip4(0, Cart::yyz, k, m) = PmB(2) * dip4(0, Cart::yy, k, m) -
                                         PmC(2) * dip4(0, Cart::yy, k, m + 1) +
                                         (k == 2) * nuc3(0, Cart::yy, m + 1);
              dip4(0, Cart::yzz, k, m) = PmB(1) * dip4(0, Cart::zz, k, m) -
                                         PmC(1) * dip4(0, Cart::zz, k, m + 1) +
                                         (k == 1) * nuc3(0, Cart::zz, m + 1);
              dip4(0, Cart::zzz, k, m) =
                  PmB(2) * dip4(0, Cart::zz, k, m) -
                  PmC(2) * dip4(0, Cart::zz, k, m + 1) +
                  (k == 2) * nuc3(0, Cart::zz, m + 1) +
                  2 * fak *
                      (dip4(0, Cart::z, k, m) - dip4(0, Cart::z, k, m + 1));
            }
          }
          //------------------------------------------------------

          // Integrals     p - f     d - f     f - f     g - f
          for (Index m = 0; m < lmax_col - 2; m++) {
            for (Index i = 1; i < n_orbitals[lmax_row]; i++) {
              int nx_i = nx[i];
              int ny_i = ny[i];
              int nz_i = nz[i];
              int ilx_i = i_less_x[i];
              int ily_i = i_less_y[i];
              int ilz_i = i_less_z[i];
              for (Index k = 0; k < 3; k++) {
                double term_x =
                    2 * fak *
                    (dip4(i, Cart::x, k, m) - dip4(i, Cart::x, k, m + 1));
                double term_y =
                    2 * fak *
                    (dip4(i, Cart::y, k, m) - dip4(i, Cart::y, k, m + 1));
                double term_z =
                    2 * fak *
                    (dip4(i, Cart::z, k, m) - dip4(i, Cart::z, k, m + 1));
                dip4(i, Cart::xxx, k, m) =
                    PmB(0) * dip4(i, Cart::xx, k, m) -
                    PmC(0) * dip4(i, Cart::xx, k, m + 1) +
                    (k == 0) * nuc3(i, Cart::xx, m + 1) +
                    nx_i * fak *
                        (dip4(ilx_i, Cart::xx, k, m) -
                         dip4(ilx_i, Cart::xx, k, m + 1)) +
                    term_x;
                dip4(i, Cart::xxy, k, m) =
                    PmB(1) * dip4(i, Cart::xx, k, m) -
                    PmC(1) * dip4(i, Cart::xx, k, m + 1) +
                    (k == 1) * nuc3(i, Cart::xx, m + 1) +
                    ny_i * fak *
                        (dip4(ily_i, Cart::xx, k, m) -
                         dip4(ily_i, Cart::xx, k, m + 1));
                dip4(i, Cart::xxz, k, m) =
                    PmB(2) * dip4(i, Cart::xx, k, m) -
                    PmC(2) * dip4(i, Cart::xx, k, m + 1) +
                    (k == 2) * nuc3(i, Cart::xx, m + 1) +
                    nz_i * fak *
                        (dip4(ilz_i, Cart::xx, k, m) -
                         dip4(ilz_i, Cart::xx, k, m + 1));
                dip4(i, Cart::xyy, k, m) =
                    PmB(0) * dip4(i, Cart::yy, k, m) -
                    PmC(0) * dip4(i, Cart::yy, k, m + 1) +
                    (k == 0) * nuc3(i, Cart::yy, m + 1) +
                    nx_i * fak *
                        (dip4(ilx_i, Cart::yy, k, m) -
                         dip4(ilx_i, Cart::yy, k, m + 1));
                dip4(i, Cart::xyz, k, m) =
                    PmB(0) * dip4(i, Cart::yz, k, m) -
                    PmC(0) * dip4(i, Cart::yz, k, m + 1) +
                    (k == 0) * nuc3(i, Cart::yz, m + 1) +
                    nx_i * fak *
                        (dip4(ilx_i, Cart::yz, k, m) -
                         dip4(ilx_i, Cart::yz, k, m + 1));
                dip4(i, Cart::xzz, k, m) =
                    PmB(0) * dip4(i, Cart::zz, k, m) -
                    PmC(0) * dip4(i, Cart::zz, k, m + 1) +
                    (k == 0) * nuc3(i, Cart::zz, m + 1) +
                    nx_i * fak *
                        (dip4(ilx_i, Cart::zz, k, m) -
                         dip4(ilx_i, Cart::zz, k, m + 1));
                dip4(i, Cart::yyy, k, m) =
                    PmB(1) * dip4(i, Cart::yy, k, m) -
                    PmC(1) * dip4(i, Cart::yy, k, m + 1) +
                    (k == 1) * nuc3(i, Cart::yy, m + 1) +
                    ny_i * fak *
                        (dip4(ily_i, Cart::yy, k, m) -
                         dip4(ily_i, Cart::yy, k, m + 1)) +
                    term_y;
                dip4(i, Cart::yyz, k, m) =
                    PmB(2) * dip4(i, Cart::yy, k, m) -
                    PmC(2) * dip4(i, Cart::yy, k, m + 1) +
                    (k == 2) * nuc3(i, Cart::yy, m + 1) +
                    nz_i * fak *
                        (dip4(ilz_i, Cart::yy, k, m) -
                         dip4(ilz_i, Cart::yy, k, m + 1));
                dip4(i, Cart::yzz, k, m) =
                    PmB(1) * dip4(i, Cart::zz, k, m) -
                    PmC(1) * dip4(i, Cart::zz, k, m + 1) +
                    (k == 1) * nuc3(i, Cart::zz, m + 1) +
                    ny_i * fak *
                        (dip4(ily_i, Cart::zz, k, m) -
                         dip4(ily_i, Cart::zz, k, m + 1));
                dip4(i, Cart::zzz, k, m) =
                    PmB(2) * dip4(i, Cart::zz, k, m) -
                    PmC(2) * dip4(i, Cart::zz, k, m + 1) +
                    (k == 2) * nuc3(i, Cart::zz, m + 1) +
                    nz_i * fak *
                        (dip4(ilz_i, Cart::zz, k, m) -
                         dip4(ilz_i, Cart::zz, k, m + 1)) +
                    term_z;
              }
            }
          }
          //------------------------------------------------------

        }  // end if (lmax_col > 2)

        if (lmax_col > 3) {

          // Integrals     s - g
          for (Index m = 0; m < lmax_col - 3; m++) {
            for (Index k = 0; k < 3; k++) {
              double term_xx =
                  fak * (dip4(0, Cart::xx, k, m) - dip4(0, Cart::xx, k, m + 1));
              double term_yy =
                  fak * (dip4(0, Cart::yy, k, m) - dip4(0, Cart::yy, k, m + 1));
              double term_zz =
                  fak * (dip4(0, Cart::zz, k, m) - dip4(0, Cart::zz, k, m + 1));
              dip4(0, Cart::xxxx, k, m) =
                  PmB(0) * dip4(0, Cart::xxx, k, m) -
                  PmC(0) * dip4(0, Cart::xxx, k, m + 1) +
                  (k == 0) * nuc3(0, Cart::xxx, m + 1) + 3 * term_xx;
              dip4(0, Cart::xxxy, k, m) =
                  PmB(1) * dip4(0, Cart::xxx, k, m) -
                  PmC(1) * dip4(0, Cart::xxx, k, m + 1) +
                  (k == 1) * nuc3(0, Cart::xxx, m + 1);
              dip4(0, Cart::xxxz, k, m) =
                  PmB(2) * dip4(0, Cart::xxx, k, m) -
                  PmC(2) * dip4(0, Cart::xxx, k, m + 1) +
                  (k == 2) * nuc3(0, Cart::xxx, m + 1);
              dip4(0, Cart::xxyy, k, m) =
                  PmB(0) * dip4(0, Cart::xyy, k, m) -
                  PmC(0) * dip4(0, Cart::xyy, k, m + 1) +
                  (k == 0) * nuc3(0, Cart::xyy, m + 1) + term_yy;
              dip4(0, Cart::xxyz, k, m) =
                  PmB(1) * dip4(0, Cart::xxz, k, m) -
                  PmC(1) * dip4(0, Cart::xxz, k, m + 1) +
                  (k == 1) * nuc3(0, Cart::xxz, m + 1);
              dip4(0, Cart::xxzz, k, m) =
                  PmB(0) * dip4(0, Cart::xzz, k, m) -
                  PmC(0) * dip4(0, Cart::xzz, k, m + 1) +
                  (k == 0) * nuc3(0, Cart::xzz, m + 1) + term_zz;
              dip4(0, Cart::xyyy, k, m) =
                  PmB(0) * dip4(0, Cart::yyy, k, m) -
                  PmC(0) * dip4(0, Cart::yyy, k, m + 1) +
                  (k == 0) * nuc3(0, Cart::yyy, m + 1);
              dip4(0, Cart::xyyz, k, m) =
                  PmB(0) * dip4(0, Cart::yyz, k, m) -
                  PmC(0) * dip4(0, Cart::yyz, k, m + 1) +
                  (k == 0) * nuc3(0, Cart::yyz, m + 1);
              dip4(0, Cart::xyzz, k, m) =
                  PmB(0) * dip4(0, Cart::yzz, k, m) -
                  PmC(0) * dip4(0, Cart::yzz, k, m + 1) +
                  (k == 0) * nuc3(0, Cart::yzz, m + 1);
              dip4(0, Cart::xzzz, k, m) =
                  PmB(0) * dip4(0, Cart::zzz, k, m) -
                  PmC(0) * dip4(0, Cart::zzz, k, m + 1) +
                  (k == 0) * nuc3(0, Cart::zzz, m + 1);
              dip4(0, Cart::yyyy, k, m) =
                  PmB(1) * dip4(0, Cart::yyy, k, m) -
                  PmC(1) * dip4(0, Cart::yyy, k, m + 1) +
                  (k == 1) * nuc3(0, Cart::yyy, m + 1) + 3 * term_yy;
              dip4(0, Cart::yyyz, k, m) =
                  PmB(2) * dip4(0, Cart::yyy, k, m) -
                  PmC(2) * dip4(0, Cart::yyy, k, m + 1) +
                  (k == 2) * nuc3(0, Cart::yyy, m + 1);
              dip4(0, Cart::yyzz, k, m) =
                  PmB(1) * dip4(0, Cart::yzz, k, m) -
                  PmC(1) * dip4(0, Cart::yzz, k, m + 1) +
                  (k == 1) * nuc3(0, Cart::yzz, m + 1) + term_zz;
              dip4(0, Cart::yzzz, k, m) =
                  PmB(1) * dip4(0, Cart::zzz, k, m) -
                  PmC(1) * dip4(0, Cart::zzz, k, m + 1) +
                  (k == 1) * nuc3(0, Cart::zzz, m + 1);
              dip4(0, Cart::zzzz, k, m) =
                  PmB(2) * dip4(0, Cart::zzz, k, m) -
                  PmC(2) * dip4(0, Cart::zzz, k, m + 1) +
                  (k == 2) * nuc3(0, Cart::zzz, m + 1) + 3 * term_zz;
            }
          }
          //------------------------------------------------------

          // Integrals     p - g     d - g     f - g     g - g
          for (Index m = 0; m < lmax_col - 3; m++) {
            for (Index i = 1; i < n_orbitals[lmax_row]; i++) {
              int nx_i = nx[i];
              int ny_i = ny[i];
              int nz_i = nz[i];
              int ilx_i = i_less_x[i];
              int ily_i = i_less_y[i];
              int ilz_i = i_less_z[i];
              for (Index k = 0; k < 3; k++) {
                double term_xx = fak * (dip4(i, Cart::xx, k, m) -
                                        dip4(i, Cart::xx, k, m + 1));
                double term_yy = fak * (dip4(i, Cart::yy, k, m) -
                                        dip4(i, Cart::yy, k, m + 1));
                double term_zz = fak * (dip4(i, Cart::zz, k, m) -
                                        dip4(i, Cart::zz, k, m + 1));
                dip4(i, Cart::xxxx, k, m) =
                    PmB(0) * dip4(i, Cart::xxx, k, m) -
                    PmC(0) * dip4(i, Cart::xxx, k, m + 1) +
                    (k == 0) * nuc3(i, Cart::xxx, m + 1) +
                    nx_i * fak *
                        (dip4(ilx_i, Cart::xxx, k, m) -
                         dip4(ilx_i, Cart::xxx, k, m + 1)) +
                    3 * term_xx;
                dip4(i, Cart::xxxy, k, m) =
                    PmB(1) * dip4(i, Cart::xxx, k, m) -
                    PmC(1) * dip4(i, Cart::xxx, k, m + 1) +
                    (k == 1) * nuc3(i, Cart::xxx, m + 1) +
                    ny_i * fak *
                        (dip4(ily_i, Cart::xxx, k, m) -
                         dip4(ily_i, Cart::xxx, k, m + 1));
                dip4(i, Cart::xxxz, k, m) =
                    PmB(2) * dip4(i, Cart::xxx, k, m) -
                    PmC(2) * dip4(i, Cart::xxx, k, m + 1) +
                    (k == 2) * nuc3(i, Cart::xxx, m + 1) +
                    nz_i * fak *
                        (dip4(ilz_i, Cart::xxx, k, m) -
                         dip4(ilz_i, Cart::xxx, k, m + 1));
                dip4(i, Cart::xxyy, k, m) =
                    PmB(0) * dip4(i, Cart::xyy, k, m) -
                    PmC(0) * dip4(i, Cart::xyy, k, m + 1) +
                    (k == 0) * nuc3(i, Cart::xyy, m + 1) +
                    nx_i * fak *
                        (dip4(ilx_i, Cart::xyy, k, m) -
                         dip4(ilx_i, Cart::xyy, k, m + 1)) +
                    term_yy;
                dip4(i, Cart::xxyz, k, m) =
                    PmB(1) * dip4(i, Cart::xxz, k, m) -
                    PmC(1) * dip4(i, Cart::xxz, k, m + 1) +
                    (k == 1) * nuc3(i, Cart::xxz, m + 1) +
                    ny_i * fak *
                        (dip4(ily_i, Cart::xxz, k, m) -
                         dip4(ily_i, Cart::xxz, k, m + 1));
                dip4(i, Cart::xxzz, k, m) =
                    PmB(0) * dip4(i, Cart::xzz, k, m) -
                    PmC(0) * dip4(i, Cart::xzz, k, m + 1) +
                    (k == 0) * nuc3(i, Cart::xzz, m + 1) +
                    nx_i * fak *
                        (dip4(ilx_i, Cart::xzz, k, m) -
                         dip4(ilx_i, Cart::xzz, k, m + 1)) +
                    term_zz;
                dip4(i, Cart::xyyy, k, m) =
                    PmB(0) * dip4(i, Cart::yyy, k, m) -
                    PmC(0) * dip4(i, Cart::yyy, k, m + 1) +
                    (k == 0) * nuc3(i, Cart::yyy, m + 1) +
                    nx_i * fak *
                        (dip4(ilx_i, Cart::yyy, k, m) -
                         dip4(ilx_i, Cart::yyy, k, m + 1));
                dip4(i, Cart::xyyz, k, m) =
                    PmB(0) * dip4(i, Cart::yyz, k, m) -
                    PmC(0) * dip4(i, Cart::yyz, k, m + 1) +
                    (k == 0) * nuc3(i, Cart::yyz, m + 1) +
                    nx_i * fak *
                        (dip4(ilx_i, Cart::yyz, k, m) -
                         dip4(ilx_i, Cart::yyz, k, m + 1));
                dip4(i, Cart::xyzz, k, m) =
                    PmB(0) * dip4(i, Cart::yzz, k, m) -
                    PmC(0) * dip4(i, Cart::yzz, k, m + 1) +
                    (k == 0) * nuc3(i, Cart::yzz, m + 1) +
                    nx_i * fak *
                        (dip4(ilx_i, Cart::yzz, k, m) -
                         dip4(ilx_i, Cart::yzz, k, m + 1));
                dip4(i, Cart::xzzz, k, m) =
                    PmB(0) * dip4(i, Cart::zzz, k, m) -
                    PmC(0) * dip4(i, Cart::zzz, k, m + 1) +
                    (k == 0) * nuc3(i, Cart::zzz, m + 1) +
                    nx_i * fak *
                        (dip4(ilx_i, Cart::zzz, k, m) -
                         dip4(ilx_i, Cart::zzz, k, m + 1));
                dip4(i, Cart::yyyy, k, m) =
                    PmB(1) * dip4(i, Cart::yyy, k, m) -
                    PmC(1) * dip4(i, Cart::yyy, k, m + 1) +
                    (k == 1) * nuc3(i, Cart::yyy, m + 1) +
                    ny_i * fak *
                        (dip4(ily_i, Cart::yyy, k, m) -
                         dip4(ily_i, Cart::yyy, k, m + 1)) +
                    3 * term_yy;
                dip4(i, Cart::yyyz, k, m) =
                    PmB(2) * dip4(i, Cart::yyy, k, m) -
                    PmC(2) * dip4(i, Cart::yyy, k, m + 1) +
                    (k == 2) * nuc3(i, Cart::yyy, m + 1) +
                    nz_i * fak *
                        (dip4(ilz_i, Cart::yyy, k, m) -
                         dip4(ilz_i, Cart::yyy, k, m + 1));
                dip4(i, Cart::yyzz, k, m) =
                    PmB(1) * dip4(i, Cart::yzz, k, m) -
                    PmC(1) * dip4(i, Cart::yzz, k, m + 1) +
                    (k == 1) * nuc3(i, Cart::yzz, m + 1) +
                    ny_i * fak *
                        (dip4(ily_i, Cart::yzz, k, m) -
                         dip4(ily_i, Cart::yzz, k, m + 1)) +
                    term_zz;
                dip4(i, Cart::yzzz, k, m) =
                    PmB(1) * dip4(i, Cart::zzz, k, m) -
                    PmC(1) * dip4(i, Cart::zzz, k, m + 1) +
                    (k == 1) * nuc3(i, Cart::zzz, m + 1) +
                    ny_i * fak *
                        (dip4(ily_i, Cart::zzz, k, m) -
                         dip4(ily_i, Cart::zzz, k, m + 1));
                dip4(i, Cart::zzzz, k, m) =
                    PmB(2) * dip4(i, Cart::zzz, k, m) -
                    PmC(2) * dip4(i, Cart::zzz, k, m + 1) +
                    (k == 2) * nuc3(i, Cart::zzz, m + 1) +
                    nz_i * fak *
                        (dip4(ilz_i, Cart::zzz, k, m) -
                         dip4(ilz_i, Cart::zzz, k, m + 1)) +
                    3 * term_zz;
              }
            }
          }
          //------------------------------------------------------

        }  // end if (lmax_col > 3)

        std::vector<Eigen::MatrixXd> multipole;

        // multipole.push_back(
        //     Eigen::Map<Eigen::MatrixXd>(nuc3.data(), nrows, ncols));
        // multipole.push_back(
        //     Eigen::Map<Eigen::MatrixXd>(nuc3.data(), nrows, ncols));
        // multipole.push_back(
        //     Eigen::Map<Eigen::MatrixXd>(nuc3.data(), nrows, ncols));

        multipole.push_back(
            Eigen::Map<Eigen::MatrixXd>(dip4.data(), nrows, ncols));
        size_t offset = nrows * ncols;
        multipole.push_back(
            Eigen::Map<Eigen::MatrixXd>(dip4.data() + offset, nrows, ncols));
        multipole.push_back(Eigen::Map<Eigen::MatrixXd>(
            dip4.data() + 2 * offset, nrows, ncols));

        std::vector<Eigen::MatrixXd> multipole_sph;
        multipole_sph.push_back(
            AOTransform::getTrafo(gaussian_row).transpose() *
            multipole[0].bottomRightCorner(shell_row.getCartesianNumFunc(),
                                           shell_col.getCartesianNumFunc()) *
            AOTransform::getTrafo(gaussian_col));
        multipole_sph.push_back(
            AOTransform::getTrafo(gaussian_row).transpose() *
            multipole[1].bottomRightCorner(shell_row.getCartesianNumFunc(),
                                           shell_col.getCartesianNumFunc()) *
            AOTransform::getTrafo(gaussian_col));
        multipole_sph.push_back(
            AOTransform::getTrafo(gaussian_row).transpose() *
            multipole[2].bottomRightCorner(shell_row.getCartesianNumFunc(),
                                           shell_col.getCartesianNumFunc()) *
            AOTransform::getTrafo(gaussian_col));
        // save to matrix

        matrix[0] += multipole_sph[0];
        matrix[1] += multipole_sph[1];
        matrix[2] += multipole_sph[2];
      }
    }  // shell_col Gaussians
  }    // shell_row Gaussians
}

}  // namespace xtp
}  // namespace votca