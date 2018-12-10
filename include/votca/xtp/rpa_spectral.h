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

#ifndef _VOTCA_XTP_RPA_SPECTRAL_H
#define _VOTCA_XTP_RPA_SPECTRAL_H

#include <votca/xtp/eigen.h>
#include <votca/xtp/vc2index.h>

namespace votca {
namespace xtp {

class TCMatrix_gwbse;

class RPA_Spectral {
    
private:
    
    // BSE Options
    int _bse_size; // Total number of BSE energy levels

    // QP Options
    int _qp_size; // Total number of QP energy levels <= _bse_size

    // Composite Indexing
    vc2index _vc2index;
    
    // GWA Energies
    Eigen::VectorXd _gwa_energies;
    
    // Spectral decomposition
    Eigen::VectorXd _Omega; // Eigenvalues
    Eigen::MatrixXd _XpY; // Eigenvector components (X + Y)
    
    // Compute (A + B) and (A - B)
    void Fill_AB(const TCMatrix_gwbse& Mmn, Eigen::MatrixXd& ApB, Eigen::VectorXd& AmB);
    
    // Do the spectral decomposition
    void Diag_C(Eigen::VectorXd& AmB_sqrt, Eigen::MatrixXd& C);
    
public:

    RPA_Spectral() : _vc2index(0, 0, 0) { // TODO: Pass vmin, cmin, ctotal as input?
        _gwa_energies.resize(0);
    }

    void configure_bse(int homo, int vmin, int cmax) {

        // TODO: Which of these values should we keep as member?
        int bse_homo = homo;
        int bse_vmin = vmin;
        int bse_vmax = homo;
        int bse_cmin = homo + 1;
        int bse_cmax = cmax;
        int bse_vtotal = bse_vmax - bse_vmin + 1;
        int bse_ctotal = bse_cmax - bse_cmin + 1;
        _bse_size = bse_vtotal * bse_ctotal;
        _vc2index = vc2index(bse_vmin, bse_cmin, bse_ctotal);
        
        return;

    }
    
    void configure_qp(int homo, int min, int max) {

        // TODO: Which of these values should we keep as member?
        int qp_homo = homo;
        int qp_min = min;
        int qp_max = max;
        _qp_size = qp_max - qp_min + 1;
        
        return;
        
    }
    
    const Eigen::VectorXd& getGWAEnergies() const {
        return _gwa_energies;
    }
    
    void set_GWAEnergies(const Eigen::VectorXd& gwa_energies) {
        _gwa_energies = gwa_energies;
    }
    
    const Eigen::VectorXd& get_Omega() const {
        return _Omega;
    }

    void prepare_decomp(const TCMatrix_gwbse& Mmn);
    
    // Compute residues (eq. 45)
    void compute_residues(const TCMatrix_gwbse& Mmn, int s, Eigen::MatrixXd& res) const;

    void FreeMatrices() {
        
        // TODO

        return;
    }
    
};
}
}

#endif /* _VOTCA_XTP_RPA_SPECTRAL_H */
