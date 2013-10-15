#include <votca/ctp/pewald3d.h>
#include <boost/format.hpp>
#include <algorithm>
#include <boost/date_time/posix_time/posix_time.hpp>
//#include <boost/timer/timer.hpp>





namespace votca { namespace ctp {

using boost::format;
    

PEwald3D3D::~PEwald3D3D() { ; }
    
    
PEwald3D3D::PEwald3D3D(Topology *top, PolarTop *ptop, Property *opt, Logger *log) 
  : Ewald3DnD(top, ptop, opt, log) {
    _shape = opt->get("options.ewald.coulombmethod.shape").as<string>();
}


void PEwald3D3D::GenerateKVectors(vector<PolarSeg*> &ps1, vector<PolarSeg*> &ps2) {
    
    // Take care of norm for grading function
    // All three components non-zero
    //              S(kx)*S(ky)*S(kz)
    // G = A(k) * ---------------------
    //            (<S(kx)><S(ky)><S(kz)>)**(2/3)
    // Component i zero
    //                   S(kj)*S(kk)
    // G = A(k) * -------------------------
    //             (<S(kj)><S(kk)>)**(1/2)
    // Components i,j zero
    // => All S(k) calculated anyway, no need to grade
    // We can use the same grading function if we set
    //
    // S(ki=0) = <S(ki)>**(2/3) (<S(kj)><S(kk)>)**(1/6)
    
    assert(!_did_generate_kvectors);
    
    vector< EWD::KVector > kvecs_2_0; // 2 components zero
    vector< EWD::KVector > kvecs_1_0; // 1 component zero
    vector< EWD::KVector > kvecs_0_0; // 0 components zero
    
    // CONTAINERS FOR GRADING K-VECTORS
    vector< double > kx_s1s2;
    kx_s1s2.push_back(1);
    vector< double > ky_s1s2;
    ky_s1s2.push_back(1);
    vector< double > kz_s1s2;
    kz_s1s2.push_back(1);
    double avg_kx_s1s2 = 0.0;
    double avg_ky_s1s2 = 0.0;
    double avg_kz_s1s2 = 0.0;
    
    // TWO COMPONENTS ZERO, ONE NON-ZERO
    LOG(logINFO,*_log) << flush 
        << "Generating K-vectors: Exploring K resonances" << flush;
    for (int i = 1; i < _NA_max+1; ++i) {
        vec k = +i*_A;
        EWD::triple<EWD::cmplx> ppuu_posk = _ewdactor.S1S2(k, ps1, ps2);        
        kx_s1s2.push_back(0.5*std::abs(ppuu_posk._pp._re));
        avg_kx_s1s2 += 0.5*std::abs(ppuu_posk._pp._re);
        EWD::KVector kvec_pos = EWD::KVector(+1*k,0.);
        EWD::KVector kvec_neg = EWD::KVector(-1*k,0.);
        kvecs_2_0.push_back(kvec_pos);
        kvecs_2_0.push_back(kvec_neg);
    }
    avg_kx_s1s2 /= _NA_max;
    
    for (int i = 1; i < _NB_max+1; ++i) {
        vec k = +i*_B;
        EWD::triple<EWD::cmplx> ppuu_posk = _ewdactor.S1S2(k, ps1, ps2);        
        ky_s1s2.push_back(0.5*std::abs(ppuu_posk._pp._re));
        avg_ky_s1s2 += 0.5*std::abs(ppuu_posk._pp._re);
        EWD::KVector kvec_pos = EWD::KVector(+1*k,0);
        EWD::KVector kvec_neg = EWD::KVector(-1*k,0);
        kvecs_2_0.push_back(kvec_pos);
        kvecs_2_0.push_back(kvec_neg);
    }
    avg_ky_s1s2 /= _NB_max;
    
    for (int i = 1; i < _NC_max+1; ++i) {
        vec k = +i*_C;
        EWD::triple<EWD::cmplx> ppuu_posk = _ewdactor.S1S2(k, ps1, ps2);        
        kz_s1s2.push_back(0.5*std::abs(ppuu_posk._pp._re));
        avg_kz_s1s2 += 0.5*std::abs(ppuu_posk._pp._re);
        EWD::KVector kvec_pos = EWD::KVector(+1*k,0);
        EWD::KVector kvec_neg = EWD::KVector(-1*k,0);
        kvecs_2_0.push_back(kvec_pos);
        kvecs_2_0.push_back(kvec_neg);
    }
    avg_kz_s1s2 /= _NC_max;
    
    double kxyz_s1s2_norm = 1./pow(avg_kx_s1s2*avg_ky_s1s2*avg_kz_s1s2,2./3.) * EWD::int2eV / _LxLyLz;
    kx_s1s2[0] = pow(avg_ky_s1s2*avg_kz_s1s2,1./6.)*pow(avg_kx_s1s2,2./3.);
    ky_s1s2[0] = pow(avg_kz_s1s2*avg_kx_s1s2,1./6.)*pow(avg_ky_s1s2,2./3.);
    kz_s1s2[0] = pow(avg_kx_s1s2*avg_ky_s1s2,1./6.)*pow(avg_kz_s1s2,2./3.);
    
    // ONE COMPONENT ZERO, TWO NON-ZERO
    LOG(logINFO,*_log)
        << "K-planes through origin: Applying K resonances" << flush;
    
    vector< EWD::KVector >::iterator kvit;
    int kx, ky, kz;
    kx = 0;
    for (ky = -_NB_max; ky < _NB_max+1; ++ky) {
        if (ky == 0) continue;
        for (kz = -_NC_max; kz < _NC_max+1; ++kz) {
            if (kz == 0) continue;
            vec k = kx*_A + ky*_B + kz*_C;
            double grade = _ewdactor.Ark2Expk2(k) * kx_s1s2[std::abs(kx)] * ky_s1s2[std::abs(ky)] * kz_s1s2[std::abs(kz)] * kxyz_s1s2_norm;
            EWD::KVector kvec = EWD::KVector(k,grade);
            kvecs_1_0.push_back(kvec);
        }
    }
    ky = 0;
    for (kx = -_NA_max; kx < _NA_max+1; ++kx) {
        if (kx == 0) continue;
        for (kz = -_NC_max; kz < _NC_max+1; ++kz) {
            if (kz == 0) continue;
            vec k = kx*_A + ky*_B + kz*_C;
            double grade = _ewdactor.Ark2Expk2(k) * kx_s1s2[std::abs(kx)] * ky_s1s2[std::abs(ky)] * kz_s1s2[std::abs(kz)] * kxyz_s1s2_norm;
            EWD::KVector kvec = EWD::KVector(k,grade);
            kvecs_1_0.push_back(kvec);
        }
    }
    kz = 0;
    for (kx = -_NA_max; kx < _NA_max+1; ++kx) {
        if (kx == 0) continue;
        for (ky = -_NB_max; ky < _NB_max+1; ++ky) {
            if (ky == 0) continue;
            vec k = kx*_A + ky*_B + kz*_C;
            double grade = _ewdactor.Ark2Expk2(k) * kx_s1s2[std::abs(kx)] * ky_s1s2[std::abs(ky)] * kz_s1s2[std::abs(kz)] * kxyz_s1s2_norm;
            EWD::KVector kvec = EWD::KVector(k,grade);
            kvecs_1_0.push_back(kvec);
        }
    }
    _kvecsort._p = 1e-300;
    std::sort(kvecs_1_0.begin(), kvecs_1_0.end(), _kvecsort);
    //for (kvit = kvecs_1_0.begin(); kvit < kvecs_1_0.end(); ++kvit) {
    //    EWD::KVector kvec = *kvit;
    //    cout << endl << std::scientific << kvec.getX() << " " << kvec.getY() << " " << kvec.getZ() << " grade " << kvec.getGrade() << flush;
    //}
    
    
    // ZERO COMPONENTS ZERO, THREE NON-ZERO
    LOG(logINFO,*_log)
        << "K-space (off-axis): Applying K resonances" << flush;
    
    for (kx = -_NA_max; kx < _NA_max+1; ++kx) {
        if (kx == 0) continue;
        for (ky = -_NB_max; ky < _NB_max+1; ++ky) {
            if (ky == 0) continue;
            for (kz = -_NC_max; kz < _NC_max+1; ++kz) {
                if (kz == 0) continue;
                vec k = kx*_A + ky*_B + kz*_C;
                double grade = _ewdactor.Ark2Expk2(k) * kx_s1s2[std::abs(kx)] * ky_s1s2[std::abs(ky)] * kz_s1s2[std::abs(kz)] * kxyz_s1s2_norm;
                EWD::KVector kvec = EWD::KVector(k,grade);
                kvecs_0_0.push_back(kvec);
            }
        }    
    }
    
    _kvecsort._p = 1e-300;
    std::sort(kvecs_0_0.begin(), kvecs_0_0.end(), _kvecsort);
    //for (kvit = kvecs_0_0.begin(); kvit < kvecs_0_0.end(); ++kvit) {
    //    EWD::KVector kvec = *kvit;
    //    cout << endl << std::scientific << kvec.getX() << " " << kvec.getY() << " " << kvec.getZ() << " grade " << kvec.getGrade() << flush;
    //}    
    
    _kvecs_2_0.clear();
    _kvecs_1_0.clear();
    _kvecs_0_0.clear();
    
    _kvecs_2_0 = kvecs_2_0;
    _kvecs_1_0 = kvecs_1_0;
    _kvecs_0_0 = kvecs_0_0;
    _kxyz_s1s2_norm = kxyz_s1s2_norm;
    
    _did_generate_kvectors = true;
    return;
}


EWD::triple<> PEwald3D3D::ConvergeRealSpaceSum() {
    
    double sum = 0.0;
    double sum_pp = 0.0;
    double sum_pu = 0.0;
    double sum_uu = 0.0;
    _converged_R = false;
    
    LOG(logDEBUG,*_log) << flush;

    vector<PolarSeg*>::iterator sit1; 
    vector<APolarSite*> ::iterator pit1;
    vector<PolarSeg*>::iterator sit2; 
    vector<APolarSite*> ::iterator pit2;
    vector< vector<PolarSeg*> > ::iterator vsit;
    
    // GENERATE MIDGROUND & ASSEMBLE IT INTO SHELLS
    double dR_shell = 0.5;
    double R_overhead = 1.1;
    double R_add = 3;
    this->SetupMidground(R_overhead*_R_co+_polar_cutoff+R_add);
    
    vector< vector<PolarSeg*> > shelled_mg_N;
    int N_shells = int((R_overhead*_R_co+_polar_cutoff+R_add)/dR_shell)+1;
    shelled_mg_N.resize(N_shells);
    
    for (sit1 = _mg_N.begin(); sit1 != _mg_N.end(); ++sit1) {
        double R = votca::tools::abs((*sit1)->getPos()-_center);
        int shell_idx = int(R/dR_shell);
        shelled_mg_N[shell_idx].push_back(*sit1);
    }

    //boost::timer::auto_cpu_timer t0(*_log);
    //t0.start();
    
    // SUM OVER CONSECUTIVE SHELLS
    bool converged = false;
    int shell_idx_cutoff = 
        (_did_field_pin_R_shell_idx) ? _field_R_shell_idx+1 : N_shells;
    
    if (_did_field_pin_R_shell_idx) LOG(logDEBUG,*_log)
            << (format("NOTE Field calculation pinned R-shell index to %1$d") 
                % _field_R_shell_idx) << flush;
    for (int sidx = 0; sidx < shell_idx_cutoff; ++sidx) {
        
        vector<PolarSeg*> &shell_mg = shelled_mg_N[sidx];
        double shell_sum = 0.0;
        double shell_term = 0.0;
        double shell_rms = 0.0;
        double shell_R = (sidx+1)*dR_shell;
        int shell_count = 0;
        
        if (shell_mg.size() < 1) continue;
        
        EWD::triple<double> ppuu(0,0,0);
        for (sit1 = _fg_C.begin(); sit1 < _fg_C.end(); ++sit1) {
            for (sit2 = shell_mg.begin(); sit2 < shell_mg.end(); ++sit2) {
                for (pit1 = (*sit1)->begin(); pit1 < (*sit1)->end(); ++pit1) {
                    for (pit2 = (*sit2)->begin(); pit2 < (*sit2)->end(); ++pit2) {
                        ppuu = _ewdactor.U12_ERFC(*(*pit1), *(*pit2));
                        sum_pp += ppuu._pp;
                        sum_pu += ppuu._pu;
                        sum_uu += ppuu._uu;
                        shell_term = ppuu._pp + ppuu._pu + ppuu._uu;
                        shell_sum += shell_term;
                        shell_rms += shell_term*shell_term;
                        shell_count += 1;
                    }
                }
            }
        }
        shell_rms = sqrt(shell_rms/shell_count)*EWD::int2eV;
        sum += shell_sum;
        LOG(logDEBUG,*_log)
            << (format("Rc = %1$+02.7f   |MGN| = %3$5d   ER = %2$+1.7f eV   dER2(sum) = %4$+1.3e eV") 
            % shell_R % (sum*EWD::int2eV) % shell_mg.size() % (shell_rms*shell_count)).str() << flush;
        
        if (shell_rms*shell_count <= _crit_dE) {
            _converged_R = true;
            converged = true;
            LOG(logDEBUG,*_log)  
                << (format(":::: Converged to precision as of Rc = %1$+1.3f nm") 
                % shell_R ) << flush;
            if (!_did_field_pin_R_shell_idx) break;
        }
    }
    if (!converged) {
        _converged_R = false;
    }
    
    //t0.stop();
    //t0.report();
    
    return EWD::triple<>(sum_pp, sum_pu, sum_uu);
    //return sum;
}


EWD::triple<> PEwald3D3D::ConvergeReciprocalSpaceSum() {
    
    if (!_did_generate_kvectors)
        this->GenerateKVectors(_fg_C, _bg_P);
    vector< EWD::KVector >::iterator kvit;
    
    double sum_re = 0.0;
    double sum_re_pp = 0.0;
    double sum_re_pu = 0.0;
    double sum_re_uu = 0.0;
    double sum_im = 0.0;
    _converged_K = false;
    
    // TWO COMPONENTS ZERO, ONE NON-ZERO
    LOG(logINFO,*_log) << flush 
        << "K-lines through origin: Checking K resonances" << flush;
    for (kvit = _kvecs_2_0.begin(); kvit < _kvecs_2_0.end(); ++kvit) {
        EWD::KVector kvec = *kvit;
        double Ak = _ewdactor.Ark2Expk2(kvec.getK());
        EWD::triple<EWD::cmplx> ppuu = _ewdactor.S1S2(kvec.getK(), _fg_C, _bg_P);        
        sum_re_pp += Ak*ppuu._pp._re;
        sum_re_pu += Ak*ppuu._pu._re;
        sum_re_uu += Ak*ppuu._uu._re;        
        sum_re += Ak*(ppuu._pp._re + ppuu._pu._re + ppuu._uu._re);        
        sum_im += Ak*(ppuu._pp._im + ppuu._pu._im + ppuu._uu._im);
    }
    
    LOG(logINFO,*_log)
        << (format("  :: RE %1$+1.7e IM %2$+1.7e")
            % (sum_re/_LxLyLz*EWD::int2eV)
            % (sum_im/_LxLyLz*EWD::int2eV)).str() << flush;
    
    // ONE COMPONENT ZERO, TWO NON-ZERO
    LOG(logINFO,*_log)
        << "K-planes through origin: Applying K resonances" << flush;
    
    double crit_grade = 1. * _kxyz_s1s2_norm;
    bool converged12 = false;
    kvit = _kvecs_1_0.begin();
    while (!converged12 && kvit < _kvecs_1_0.end()) {
        
        double de_this_shell = 0.0;
        int shell_count = 0;
        
        while (kvit < _kvecs_1_0.end()) {
            EWD::KVector kvec = *kvit;
            if (kvec.getGrade() < crit_grade) break;
            EWD::triple<EWD::cmplx> ppuu = _ewdactor.AS1S2(kvec.getK(), _fg_C, _bg_P);
            
            sum_re_pp += ppuu._pp._re;
            sum_re_pu += ppuu._pu._re;
            sum_re_uu += ppuu._uu._re;            
            sum_re += ppuu._pp._re + ppuu._pu._re + ppuu._uu._re;
            sum_im += ppuu._pp._im + ppuu._pu._im + ppuu._uu._im;
            
            //de_this_shell += sqrt(as1s2._re*as1s2._re + as1s2._im*as1s2._im);
            de_this_shell += ppuu._pp._re + ppuu._pu._re + ppuu._uu._re;
            //cout << endl << std::showpos << std::scientific 
            //   << kvec.getX() << " " << kvec.getY() << " " << kvec.getZ() 
            //   << " grade " << kvec.getGrade() << " re " << (as1s2._re/_LxLyLz*_ewdactor.int2eV) << flush;            
            ++kvit;
            ++shell_count;
        }
        de_this_shell = (de_this_shell < 0.) ? -de_this_shell : de_this_shell;
        
        LOG(logDEBUG,*_log)
             << (format("M = %1$04d   G = %2$+1.3e   dE(rms) = %3$+1.3e eV")
             % shell_count
             % crit_grade
             % (de_this_shell/_LxLyLz*EWD::int2eV)).str() << flush;
        
        if (shell_count > 10 && de_this_shell/_LxLyLz*EWD::int2eV < _crit_dE) {
            LOG(logINFO,*_log)
                << (format("  :: RE %1$+1.7e IM %2$+1.7e") 
                % (sum_re/_LxLyLz*EWD::int2eV)
                % (sum_im/_LxLyLz*EWD::int2eV)).str() << flush;
            converged12 = true;
        }
        
        crit_grade /= 10.0;
    }
    
    
    // ZERO COMPONENTS ZERO, THREE NON-ZERO
    LOG(logINFO,*_log)
        << "K-space (off-axis): Applying K resonances" << flush;    
    
    crit_grade = 1. * _kxyz_s1s2_norm;
    double converged03 = false;
    kvit = _kvecs_0_0.begin();
    while (!converged03 && kvit < _kvecs_0_0.end()) {
        
        double de_this_shell = 0.0;
        int shell_count = 0;
        
        while (kvit < _kvecs_0_0.end()) {
            EWD::KVector kvec = *kvit;
            if (kvec.getGrade() < crit_grade) break;
            EWD::triple<EWD::cmplx> ppuu = _ewdactor.AS1S2(kvec.getK(), _fg_C, _bg_P);
            
            sum_re_pp += ppuu._pp._re;
            sum_re_pu += ppuu._pu._re;
            sum_re_uu += ppuu._uu._re;            
            sum_re += ppuu._pp._re + ppuu._pu._re + ppuu._uu._re;
            sum_im += ppuu._pp._im + ppuu._pu._im + ppuu._uu._im;

            //de_this_shell += sqrt(as1s2._re*as1s2._re + as1s2._im*as1s2._im);
            de_this_shell += ppuu._pp._re + ppuu._pu._re + ppuu._uu._re;
            //cout << endl << std::showpos << std::scientific 
            //   << kvec.getX() << " " << kvec.getY() << " " << kvec.getZ() 
            //   << " grade " << kvec.getGrade() << " re " << (as1s2._re/_LxLyLz*_ewdactor.int2eV) << flush;            
            ++kvit;
            ++shell_count;
        }
        de_this_shell = (de_this_shell < 0.) ? -de_this_shell : de_this_shell;
        
        LOG(logDEBUG,*_log)
             << (format("M = %1$04d   G = %2$+1.3e   dE(rms) = %3$+1.3e eV")
             % shell_count
             % crit_grade
             % (de_this_shell/_LxLyLz*EWD::int2eV)).str() << flush;
        
        if (shell_count > 10 && de_this_shell/_LxLyLz*EWD::int2eV < _crit_dE) {
            LOG(logINFO,*_log)
                << (format("  :: RE %1$+1.7e IM %2$+1.7e") 
                % (sum_re/_LxLyLz*EWD::int2eV)
                % (sum_im/_LxLyLz*EWD::int2eV)).str() << flush;
            converged03 = true;
        }
        
        crit_grade /= 10.0;
    }
    
    _converged_K = converged12 && converged03;
    
    if (_converged_K)
        LOG(logINFO,*_log)
            << (format(":::: Converged to precision, {0-2}, {1-2}, {0-3}."))
            << flush;
    else ;

    return EWD::triple<>(sum_re_pp/_LxLyLz, sum_re_pu/_LxLyLz, sum_re_uu/_LxLyLz);
}


EWD::triple<> PEwald3D3D::CalculateShapeCorrection() {
    
    vector<PolarSeg*>::iterator sit1; 
    vector<APolarSite*> ::iterator pit1;
    vector<PolarSeg*>::iterator sit2; 
    vector<APolarSite*> ::iterator pit2;
    
    double EJ = 0.0;
    double sum_pp = 0.0;
    double sum_pu = 0.0;
    double sum_uu = 0.0;
    
    if (_shape == "xyslab") {
        EWD::triple<double> ppuu(0,0,0);
        for (sit1 = _fg_C.begin(); sit1 < _fg_C.end(); ++sit1) {
           for (sit2 = _bg_P.begin(); sit2 < _bg_P.end(); ++sit2) {
              for (pit1 = (*sit1)->begin(); pit1 < (*sit1)->end(); ++pit1) {
                 for (pit2 = (*sit2)->begin(); pit2 < (*sit2)->end(); ++pit2) {
                    ppuu = _ewdactor.U12_XYSlab(*(*pit1), *(*pit2));
                    sum_pp += ppuu._pp;
                    sum_pu += ppuu._pu;
                    sum_uu += ppuu._uu;
                 }
              }
           }
        }
        sum_pp *= -2*M_PI/_LxLyLz;
        sum_pu *= -2*M_PI/_LxLyLz;
        sum_uu *= -2*M_PI/_LxLyLz;
        EJ = sum_pp + sum_pu + sum_uu;
    }
    else {
        LOG(logERROR,*_log)
            << (format("Shape %1$s not implemented. Setting EJ = 0.0 ...") 
            % _shape) << flush;
        EJ = 0.0;
    }
    
    return EWD::triple<>(sum_pp, sum_pu, sum_uu);
    //return EJ;
}


EWD::triple<> PEwald3D3D::CalculateForegroundCorrection() {
    vector<PolarSeg*>::iterator sit1; 
    vector<APolarSite*> ::iterator pit1;
    vector<PolarSeg*>::iterator sit2; 
    vector<APolarSite*> ::iterator pit2;
    double EC = 0.0;
    double sum_pp = 0.0;
    double sum_pu = 0.0;
    double sum_uu = 0.0;
    
    EWD::triple<double> ppuu(0,0,0);
    for (sit1 = _fg_C.begin(); sit1 < _fg_C.end(); ++sit1) {
        for (sit2 = _fg_N.begin(); sit2 < _fg_N.end(); ++sit2) {
            for (pit1 = (*sit1)->begin(); pit1 < (*sit1)->end(); ++pit1) {
                for (pit2 = (*sit2)->begin(); pit2 < (*sit2)->end(); ++pit2) {
                    ppuu = _ewdactor.U12_ERF(*(*pit1), *(*pit2));
                    sum_pp += ppuu._pp;
                    sum_pu += ppuu._pu;
                    sum_uu += ppuu._uu;
                }
            }
        }
    }
    
    EC = sum_pp + sum_pu + sum_uu;
    return EWD::triple<>(sum_pp, sum_pu, sum_uu);
    //return EC;
}


void PEwald3D3D::Field_ConvergeRealSpaceSum() {
    
    double sum = 0.0;
    _field_converged_R = false;
    
    LOG(logDEBUG,*_log) << flush;

    vector<PolarSeg*>::iterator sit1; 
    vector<APolarSite*> ::iterator pit1;
    vector<PolarSeg*>::iterator sit2; 
    vector<APolarSite*> ::iterator pit2;
    
    // GENERATE MIDGROUND & ASSEMBLE IT INTO SHELLS
    double dR_shell = 0.5;
    double R_overhead = 1.1;
    double R_add = 3;
    this->SetupMidground(R_overhead*_R_co+_polar_cutoff+R_add);
    
    vector< vector<PolarSeg*> > shelled_mg_N;
    int N_shells = int((R_overhead*_R_co+_polar_cutoff+R_add)/dR_shell)+1;
    shelled_mg_N.resize(N_shells);
    
    for (sit1 = _mg_N.begin(); sit1 != _mg_N.end(); ++sit1) {
        double R = votca::tools::abs((*sit1)->getPos()-_center);
        int shell_idx = int(R/dR_shell);
        shelled_mg_N[shell_idx].push_back(*sit1);
    }
    
    // SUM OVER CONSECUTIVE SHELLS
    bool converged = false;
    for (int sidx = 0; sidx < N_shells; ++sidx) {
        
        _did_field_pin_R_shell_idx = true;
        _field_R_shell_idx = sidx;
        
        vector<PolarSeg*> &shell_mg = shelled_mg_N[sidx];
        double shell_rms = 0.0;
        double shell_R = (sidx+1)*dR_shell;
        int shell_count = 0;
        
        if (shell_mg.size() < 1) continue;

        for (sit1 = _fg_C.begin(); sit1 < _fg_C.end(); ++sit1) {
            for (sit2 = shell_mg.begin(); sit2 < shell_mg.end(); ++sit2) {
                for (pit1 = (*sit1)->begin(); pit1 < (*sit1)->end(); ++pit1) {
                    for (pit2 = (*sit2)->begin(); pit2 < (*sit2)->end(); ++pit2) {
                        shell_rms += _ewdactor.FPU12_ERFC_At_By(*(*pit1), *(*pit2));
                        shell_count += 1;
                        //_actor.BiasStat(*(*pit1), *(*pit2));
                        //_actor.FieldPerm(*(*pit1), *(*pit2));
                    }
                }
            }
        }
        
        shell_rms = sqrt(shell_rms/shell_count)*EWD::int2V_m;
        double e_measure = shell_rms*1e-10*shell_count; // Energy of dipole of size 0.1*e*nm summed over shell
        
        LOG(logDEBUG,*_log)
            << (format("Rc = %1$+02.7f   |MGN| = %2$5d   dF(rms) = %3$+1.3e V/m   [1eA => %4$+1.3e eV]") 
            % shell_R % shell_mg.size() % shell_rms  % e_measure).str() << flush;
        
        if (e_measure <= _crit_dE) {
            _field_converged_R = true;
            converged = true;
            LOG(logDEBUG,*_log)
                << (format(":::: Converged to precision as of Rc = %1$+1.3f nm") 
                % shell_R) << flush;
            break;
        }
    }
    if (!converged) {
        _field_converged_R = false;
    }    
    return;
}


void PEwald3D3D::Field_ConvergeReciprocalSpaceSum() {

    this->GenerateKVectors(_fg_C, _bg_P);
    double sum_re = 0.0;
    double sum_im = 0.0;
    _field_converged_K = false;
    double rV = 1./_LxLyLz;
    
    vector< EWD::KVector >::iterator kvit;
    
    // TWO COMPONENTS ZERO, ONE NON-ZERO
    LOG(logINFO,*_log) << flush 
        << "K-lines through origin: Checking K resonances" << flush;
    for (kvit = _kvecs_2_0.begin(); kvit < _kvecs_2_0.end(); ++kvit) {
        EWD::KVector kvec = *kvit;
        EWD::cmplx f_as1s2 = _ewdactor.FPU12_AS1S2_At_By(kvec.getK(), _fg_C, _bg_P, rV);
        sum_re += sqrt(f_as1s2._re);
        sum_im += f_as1s2._im;
    }
    
    LOG(logINFO,*_log)
        << (format("  :: RE %1$+1.7e IM %2$+1.7e")
            % (sum_re*EWD::int2V_m)
            % (sum_im*EWD::int2V_m)).str() << flush;
    
    // ONE COMPONENT ZERO, TWO NON-ZERO
    LOG(logINFO,*_log)
        << "K-planes through origin: Applying K resonances" << flush;    
    
    double crit_grade = 1. * _kxyz_s1s2_norm;
    bool converged12 = false;
    kvit = _kvecs_1_0.begin();
    while (!converged12 && kvit < _kvecs_1_0.end()) {
        
        double shell_rms = 0.0;
        int rms_count = 0;
        
        while (kvit < _kvecs_1_0.end()) {
            EWD::KVector kvec = *kvit;
            if (kvec.getGrade() < crit_grade) break;
            EWD::cmplx f_as1s2 = _ewdactor.FPU12_AS1S2_At_By(kvec.getK(), _fg_C, _bg_P, rV);
            sum_re += f_as1s2._re;
            sum_im += f_as1s2._im;
            shell_rms += f_as1s2._re;
            //cout << endl << std::showpos << std::scientific 
            //   << kvec.getX() << " " << kvec.getY() << " " << kvec.getZ() 
            //   << " grade " << kvec.getGrade() << " re " << (as1s2._re/_LxLyLz*_ewdactor.int2eV) << flush;            
            ++kvit;
            ++rms_count;
        }
        shell_rms = (rms_count > 0) ? sqrt(shell_rms/rms_count)*EWD::int2V_m : 0.0;
        double e_measure = shell_rms*1e-10*rms_count;
        
        if (rms_count > 0) LOG(logDEBUG,*_log)
             << (format("M = %1$04d   G = %2$+1.3e   dF(rms) = %3$+1.3e V/m   [1eA => %4$+1.3e eV]")
             % rms_count
             % crit_grade
             % shell_rms
             % e_measure).str() << flush;
        
        if (rms_count > 10 && e_measure <= _crit_dE) {
            LOG(logINFO,*_log)
                << (format("  :: RE %1$+1.7e IM %2$+1.7e") 
                % (sqrt(sum_re)*EWD::int2V_m)
                % (sum_im*EWD::int2V_m)).str() << flush;
            converged12 = true;
        }
        
        crit_grade /= 10.0;
    }
    
    // ZERO COMPONENTS ZERO, THREE NON-ZERO
    LOG(logINFO,*_log)
        << "K-space (off-axis): Applying K resonances" << flush;
    
    crit_grade = 1. * _kxyz_s1s2_norm;
    double converged03 = false;
    kvit = _kvecs_0_0.begin();
    while (!converged03 && kvit < _kvecs_0_0.end()) {
        
        double shell_rms = 0.0;
        int rms_count = 0;
        
        while (kvit < _kvecs_0_0.end()) {
            EWD::KVector kvec = *kvit;
            if (kvec.getGrade() < crit_grade) break;
            EWD::cmplx f_as1s2 = _ewdactor.FPU12_AS1S2_At_By(kvec.getK(), _fg_C, _bg_P, rV);
            sum_re += f_as1s2._re;
            sum_im += f_as1s2._im;
            shell_rms += f_as1s2._re;
            //cout << endl << std::showpos << std::scientific 
            //   << kvec.getX() << " " << kvec.getY() << " " << kvec.getZ() 
            //   << " grade " << kvec.getGrade() << " re " << (as1s2._re/_LxLyLz*_ewdactor.int2eV) << flush;            
            ++kvit;
            ++rms_count;
        }
        shell_rms = (rms_count > 0) ? sqrt(shell_rms/rms_count)*EWD::int2V_m : 0.0;
        double e_measure = shell_rms*1e-10*rms_count;
        
        if (rms_count > 0) LOG(logDEBUG,*_log)
             << (format("M = %1$04d   G = %2$+1.3e   dF(rms) = %3$+1.3e V/m   [1eA => %4$+1.3e eV]")
             % rms_count
             % crit_grade
             % shell_rms
             % e_measure).str() << flush;
        
        if (rms_count > 10 && e_measure <= _crit_dE) {
            LOG(logINFO,*_log)
                << (format("  :: RE %1$+1.7e IM %2$+1.7e") 
                % (sqrt(sum_re)*EWD::int2V_m)
                % (sum_im*EWD::int2V_m)).str() << flush;
            converged03 = true;
        }
        
        crit_grade /= 10.0;
    }
    
    _field_converged_K = converged12 && converged03;
    
    if (_field_converged_K)
        LOG(logINFO,*_log)
            << (format(":::: Converged to precision, {0-2}, {1-2}, {0-3}."))
            << flush;
    else ;

    return;
}


void PEwald3D3D::Field_CalculateForegroundCorrection() {
    
    vector<PolarSeg*>::iterator sit1; 
    vector<APolarSite*> ::iterator pit1;
    vector<PolarSeg*>::iterator sit2; 
    vector<APolarSite*> ::iterator pit2;
    
    double rms = 0.0;
    int rms_count = 0;
    for (sit1 = _fg_C.begin(); sit1 < _fg_C.end(); ++sit1) {
        for (sit2 = _fg_N.begin(); sit2 < _fg_N.end(); ++sit2) {
            for (pit1 = (*sit1)->begin(); pit1 < (*sit1)->end(); ++pit1) {
                for (pit2 = (*sit2)->begin(); pit2 < (*sit2)->end(); ++pit2) {
                    rms += _ewdactor.FPU12_ERF_At_By(*(*pit1), *(*pit2));
                    rms_count += 1;
                }
            }
        }
    }
    rms = sqrt(rms/rms_count)*EWD::int2V_m;
    
    return;
}


void PEwald3D3D::Field_CalculateShapeCorrection() {

    vector<PolarSeg*>::iterator sit1; 
    vector<APolarSite*> ::iterator pit1;
    vector<PolarSeg*>::iterator sit2; 
    vector<APolarSite*> ::iterator pit2;
    
    double rms = 0.0;
    int rms_count = 0;    
    if (_shape == "xyslab") {
        double TwoPi_V = 2*M_PI/_LxLyLz;
        
//        for (sit1 = _fg_C.begin(); sit1 < _fg_C.end(); ++sit1) {
//           for (sit2 = _bg_P.begin(); sit2 < _bg_P.end(); ++sit2) {
//              for (pit1 = (*sit1)->begin(); pit1 < (*sit1)->end(); ++pit1) {
//                 for (pit2 = (*sit2)->begin(); pit2 < (*sit2)->end(); ++pit2) {
//                    rms += _ewdactor.F12_XYSlab_At_By(*(*pit1), *(*pit2), TwoPi_V);
//                    rms_count += 1;
//                 }
//              }
//           }
//        }
        
        _ewdactor.FPU12_XYSlab_ShapeField_At_By(_fg_C, _bg_P, TwoPi_V);
        
    }
    else {
        LOG(logERROR,*_log)
            << (format("Shape %1$s not implemented. Setting EJ = 0.0 ...") 
            % _shape) << flush;
    }
    rms = sqrt(rms/rms_count)*EWD::int2V_m;
    
    return;
}


void PEwald3D3D::PolarizeBackground() {
    
    TLogLevel dbg = logDEBUG;
    TLogLevel inf = logINFO;
    TLogLevel err = logERROR;
    Logger &log = *_log;
    
    LOG(dbg,log) << flush;
    LOG(dbg,log) << "Polarize background" << flush;
    
    /*
    Verify neutrality & depolarize
    Generate permanent fields (FP)
      o Converge intermolecular real-space contribution, remember cut-off
      o Converge reciprocal-space contribution, remember K-vectors
      o Calculate shape fields
      o Apply MOLECULAR foreground correction     
    Induce to 1st order
    Loop until 2nd-order fields converged
      | Reset 2nd-order fields
      | (Re-)generate induction fields (FU)
      | o Real-space INTRAmolecular contribution to 2nd-order fields
      | o Real-space INTERmolecular contribution to 2nd-order fields
      | o Reciprocal-space contribution, work off remembered K-vectors
      | o Calculate shape fields
      | o Apply ATOMIC foreground correction
      | Induce to 2nd order
      + Check convergence
    Extract (or serialize) induction state to hard-drive
    */
    
    vector<PolarSeg*>::iterator sit1; 
    vector<APolarSite*> ::iterator pit1;
    vector<PolarSeg*>::iterator sit2; 
    vector<APolarSite*> ::iterator pit2;    
    
    // VERIFY NEUTRALITY & DEPOLARIZE
    // In principle, this was already checked in the Ewald3DnD constructor
    double Q_bg_P = 0.0;
    for (sit1 = _bg_P.begin(); sit1 < _bg_P.end(); ++sit1) {
        Q_bg_P += (*sit1)->CalcTotQ();
        for (pit1 = (*sit1)->begin(); pit1 < (*sit1)->end(); ++pit1) {
            (*pit1)->Depolarize();
        }
    }    
    if (Q_bg_P < 1e-4) {
        LOG(dbg,log) 
            << "  o Net background charge is zero (enough). Proceed." << flush;
    }
    else {
        LOG(err,log) 
            << "  o ERROR Net background charge > 1e-4. Abort." << endl;
        throw std::runtime_error("Bg charge density is not neutral (enough).");
    }

    
    
    
    
    return;
}
    
    
}}