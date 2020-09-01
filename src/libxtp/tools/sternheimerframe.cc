/*
 * Copyright 2009-2020 The VOTCA Development Team (http://www.votca.org)
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

// VOTCA includes
#include <votca/tools/constants.h>

// Local VOTCA includes
#include "votca/xtp/gwbseengine.h"
#include "votca/xtp/qmpackagefactory.h"
#include "votca/xtp/segment.h"
#include "votca/xtp/staticregion.h"


// Local private VOTCA includes
#include "sternheimerframe.h"

using std::flush;

namespace votca {
namespace xtp {

void SternheimerFrame::Initialize(const tools::Property &user_options) {

    std::string key = "sternheimer";

    _log.setReportLevel(Log::current_level);

    _log.setMultithreading(true);
    _log.setCommonPreface("\n... ...");

    tools::Property options =
      LoadDefaultsAndUpdateWithUserOptions("xtp", user_options);

    _orbfile = options.ifExistsReturnElseReturnDefault<std::string>(
      ".orb", _job_name + ".orb");

    XTP_LOG(Log::error, _log) << " Running Sternheimer" << flush;
    
    _options.start_frequency_grid=options.get(".omegain").as<double>();
    _options.end_frequency_grid=options.get(".omegafin").as<double>();
    _options.number_of_frequency_grid_points = options.get(".step").as<Index>();
    _options.imaginary_shift_pade_approx = options.get(".imshift").as<double>();
    _options.number_output_grid_points = options.get(".resolution").as<Index>();
    _options.do_precalc_fxc = options.get(".do_precalc_fxc").as<bool>();
    _options.calculation = options.get(".calculation").as<std::string>();
    _options.numerical_Integration_grid_type = options.get(".spatialgridtype").as<std::string>();
    _options.quadrature_scheme = options.get(".quadrature_scheme").as<std::string>();
    _options.quadrature_order = options.get(".quadrature_order").as<Index>();
    _options.level = options.get(".level").as<Index>();

    XTP_LOG(Log::error, _log) << " Task: " <<  _options.calculation << flush;

    XTP_LOG(Log::error, _log)
        << " Omega initial: " << _options.start_frequency_grid << flush;
    XTP_LOG(Log::error, _log) << " Omega final: " << _options.end_frequency_grid << flush;
    XTP_LOG(Log::error, _log) << " Step: " << _options.number_of_frequency_grid_points << flush;
    XTP_LOG(Log::error, _log)
        << " Imaginary shift: " << _options.imaginary_shift_pade_approx << flush;
    XTP_LOG(Log::error, _log)
        << " Resolution: " << _options.number_output_grid_points << flush;
    XTP_LOG(Log::error, _log)
        << " GW-Sternheimer level: " << _options.level << flush;
    XTP_LOG(Log::error, _log)
        << " Calculation: " << _options.calculation << flush;
    XTP_LOG(Log::error, _log)
        << " Quadrature: " << _options.quadrature_scheme << flush
        << " Order: " << _options.quadrature_order << flush << flush;

};
bool SternheimerFrame::Evaluate() {

   OPENMP::setMaxThreads(_nThreads);

  _log.setReportLevel(Log::error);
  _log.setMultithreading(true);
  _log.setCommonPreface("\n... ...");

  XTP_LOG(Log::error, _log)
      << TimeStamp() << " Reading from orbitals from file: " << _orbfile << flush;

  // Get orbitals object
  Orbitals orbitals;

  orbitals.ReadFromCpt(_orbfile);

  XTP_LOG(Log::error, _log)
       << " Orbital data: " << flush;
  XTP_LOG(Log::error, _log)
       << " Basis size: " << orbitals.getBasisSetSize() << flush;
  XTP_LOG(Log::error, _log)
       << " XC Functional: " << orbitals.getXCFunctionalName() << flush;
  XTP_LOG(Log::error, _log)
       << " Homo level: " << orbitals.getHomo() << flush;

  Sternheimer sternheimer(orbitals, &_log);

  sternheimer.configurate(_options);

  sternheimer.setUpMatrices();

  XTP_LOG(Log::error, _log)
      << TimeStamp() << " Started Sternheimer " << flush;
  if (_options.calculation == "polarizability") {
    XTP_LOG(Log::error, _log)
        << TimeStamp() << " Started Sternheimer Polarizability" << flush;
    std::vector<Eigen::Matrix3cd> polar = sternheimer.Polarisability();
    sternheimer.printIsotropicAverage(polar);
    XTP_LOG(Log::error, _log)
        << TimeStamp() << " Finished Sternheimer Polarizability" << flush;
  }
  if (_options.calculation == "gradient") {
    XTP_LOG(Log::error, _log)
        << TimeStamp() << " Started Sternheimer Energy Gradient" << flush;
    std::vector<Eigen::Vector3cd> EPC = sternheimer.EnergyGradient();
    sternheimer.printHellmannFeynmanForces(EPC);
    XTP_LOG(Log::error, _log)
        << TimeStamp() << " Finished Sternheimer Energy Gradient" << flush;
  }
  if (_options.calculation == "mogradient") {
    XTP_LOG(Log::error, _log)
        << TimeStamp() << " Started Sternheimer MO Energy Gradient" << flush;
    for (Index n = 0; n < orbitals.MOs().eigenvalues().size(); ++n) {

      std::vector<Eigen::Vector3cd> EPC = sternheimer.MOEnergyGradient(n, n);
      sternheimer.printMOEnergyGradient(EPC, n, n);
    }
    XTP_LOG(Log::error, _log)
        << TimeStamp() << " Finished Sternheimer MO Energy Gradient" << flush;
  }
  if (_options.calculation == "koopmanalpha") {
    XTP_LOG(Log::error, _log)
        << TimeStamp()
        << " Started Sternheimer Koopman's relaxation coefficients" << flush;
    for (Index n = 0; n < orbitals.MOs().eigenvalues().size(); ++n) {

      std::complex<double> alpha = sternheimer.KoopmanRelaxationCoeff(n, 2.0);
      sternheimer.printKoopmanRelaxationCoeff(alpha, n);
    }
    XTP_LOG(Log::error, _log)
        << TimeStamp()
        << " Finished Sternheimer Koopman's relaxation coefficients" << flush;
  }

  if (_options.calculation == "koopman") {
    XTP_LOG(Log::error, _log)
        << TimeStamp() << " Started Sternheimer Koopman's compliant" << flush;
    for (Index n = 0; n < orbitals.MOs().eigenvalues().size(); ++n) {

      double f = 1.0;  // For occupied states
      if (n > orbitals.getHomo()) {
        f = 0.0;  // for unoccupied state
      }
      std::complex<double> alpha = sternheimer.KoopmanRelaxationCoeff(n, 1);
      std::complex<double> correction = sternheimer.KoopmanCorrection(n, f);
      std::cout << correction + orbitals.MOs().eigenvalues()(n) << std::endl;
      sternheimer.printKoopman(alpha, correction, n);
    }
    XTP_LOG(Log::error, _log)
        << TimeStamp() << " Finished Sternheimer Koopman's compliant" << flush;
  }

  if (_options.calculation == "gwsternheimer") {
    XTP_LOG(Log::error, _log)
        << TimeStamp() << " Started Sternheimer GW" << flush;
    sternheimer.printGW(_options.level);
    XTP_LOG(Log::error, _log)
        << TimeStamp() << " Finished Sternheimer GW" << flush;
  }

  XTP_LOG(Log::error, _log)
      << TimeStamp() << " Finished Sternheimer" << flush;

  return true;
}

}  // namespace xtp
}  // namespace votca