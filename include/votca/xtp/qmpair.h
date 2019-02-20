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
/// For earlier commit history see ctp commit
/// 77795ea591b29e664153f9404c8655ba28dc14e9

#ifndef VOTCA_XTP_QMPAIR_H
#define VOTCA_XTP_QMPAIR_H

#include <vector>
#include <votca/xtp/eigen.h>
#include <votca/xtp/segment.h>

#include "qmstate.h"

namespace votca {
namespace xtp {

class QMPair {
 public:
  enum PairType { Hopping, Excitoncl };

  static std::string get_name(PairType type) {
    switch (type) {
      case Hopping:
        return "Hopping";
      case Excitoncl:
        return "Excitoncl";
        // no default case to trigger compiler error
    }
  }

  static PairType get_Enum(std::string type) {
    if (type == "Hopping") {
      return PairType::Hopping;
    } else if (type == "Excitoncl") {
      return PairType::Excitoncl;
    } else {
      throw std::runtime_error("get_Enum input is invalid");
    }
  }

  QMPair(int id, Segment* seg1, Segment* seg2, const Eigen::Vector3d& delta_R);

  QMPair(CheckpointReader& r, const std::vector<Segment>& segments) {
    ReadFromCpt(r, segments);
  }

  int getId() const { return _id; }
  const Eigen::Vector3d& R() const { return _R; }
  double Dist() const { return _R.norm(); }

  void setLambdaO(double lO, QMStateType state) {
    _lambda0.setValue(lO, state);
  }
  double getLambdaO(QMStateType state) const {
    return _lambda0.getValue(state);
  }

  double getReorg12(QMStateType state) const {
    return _segments.first->getU_nX_nN(state) +
           _segments.second->getU_xN_xX(state);
  }  // 1->2
  double getReorg21(QMStateType state) const {
    return _segments.first->getU_xN_xX(state) +
           _segments.second->getU_nX_nN(state);
  }  // 2->1

  double getJeff2(QMStateType state) const { return _Jeff2.getValue(state); }
  void setJeff2(double Jeff2, QMStateType state) {
    _Jeff2.setValue(Jeff2, state);
  }

  double getdE12(QMStateType state) {
    return _segments.second->getSiteEnergy(state) -
           _segments.first->getSiteEnergy(state);
  }

  Segment* Seg2PbCopy() const;
  Segment* Seg1() const { return _segments.first; }
  Segment* Seg2() const { return _segments.second; }

  bool HasGhost() const { return _ghost != nullptr; }

  void setType(PairType pair_type) { _pair_type = pair_type; }
  const PairType& getType() const { return _pair_type; }

  void WriteToCpt(CheckpointWriter& w) const;

  void ReadFromCpt(CheckpointReader& r, const std::vector<Segment>& segments);

 private:
  std::pair<Segment*, Segment*> _segments;

  int _id = -1;
  Eigen::Vector3d _R = Eigen::Vector3d::Zero();

  std::unique_ptr<Segment> _ghost = nullptr;
  PairType _pair_type = PairType::Hopping;

  QMStateCarrierStorage<double> _lambda0;
  QMStateCarrierStorage<double> _Jeff2;
};

}  // namespace xtp
}  // namespace votca

#endif  // VOTCA_XTP_QMPAIR_H
