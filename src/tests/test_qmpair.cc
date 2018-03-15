/*
 * Copyright 2009-2018 The VOTCA Development Team (http://www.votca.org)
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
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
#define BOOST_TEST_MAIN

#define BOOST_TEST_MODULE qmpair_test
#include <boost/test/unit_test.hpp>
#include <votca/xtp/qmpair.h>
#include <votca/xtp/topology.h>
#include <votca/xtp/segment.h>
#include <votca/xtp/atom.h>

#include <votca/tools/matrix.h>
#include <votca/tools/vec.h>

#include <votca/csg/boundarycondition.h>

using namespace votca::tools;

using namespace votca::xtp;

BOOST_AUTO_TEST_SUITE(qmpair_test)

BOOST_AUTO_TEST_CASE(constructors_test) { QMPair qm_p; }

BOOST_AUTO_TEST_CASE(getters_test) { 

  // Box takes a vector
  double x1 = 12.0;
  double y1 = 0.0;
  double z1 = 0.0;

  double x2 = 0.0;
  double y2 = 12.0;
  double z2 = 0.0;

  double x3 = 0.0;
  double y3 = 0.0;
  double z3 = 12.0;

  vec v1(x1,y1,z1);
  vec v2(x2,y2,z2);
  vec v3(x3,y3,z3);

  matrix box(v1,v2,v3);

  Topology top;
  top.setBox(box);
  vec p1;
  p1.setX(0.0);
  p1.setY(0.0);
  p1.setZ(0.0);
  
  vec p2;
  p2.setX(10.0);
  p2.setY(0.0);
  p2.setZ(0.0);
  top.PbShortestConnect(p1,p2);

  bool hasQM = true;  
  vec qmpos;
  qmpos.setX(2.0);
  qmpos.setY(2.0);
  qmpos.setZ(2.0);

  vec pos;
  pos.setX(3.0);
  pos.setY(3.0);
  pos.setZ(3.0);

  Atom * atm = new Atom(nullptr, "res1",1,"CSP",2,hasQM,3,qmpos,"C",1.0);
  atm->setPos(pos);
  Segment * seg1 = new Segment(1, "seg1");
  seg1->AddAtom(atm);
  seg1->setTopology(&top);

  vec qmpos_2;
  qmpos_2.setX(4.0);
  qmpos_2.setY(4.0);
  qmpos_2.setZ(4.0);

  vec pos_2;
  pos_2.setX(1.0);
  pos_2.setY(1.0);
  pos_2.setZ(1.0);

  Atom * atm_2 = new Atom(nullptr, "res1",1,"CSP",2,hasQM,3,qmpos_2,"H",1.0);
  atm->setPos(pos_2);
  Segment * seg2 = new Segment(2, "seg2");
  seg2->AddAtom(atm_2);
  seg2->setTopology(&top);

  int pair_num = 1;
  QMPair qm_p(pair_num,seg1,seg2); 
  BOOST_CHECK_EQUAL(qm_p.getId(),1);
  auto r = qm_p.R();
  BOOST_CHECK_EQUAL(r.x(),0.0);
  BOOST_CHECK_EQUAL(r.y(),0.0);
  BOOST_CHECK_EQUAL(r.z(),0.0);
  BOOST_CHECK_EQUAL(qm_p.HasGhost(),false);

  int state = -1;
  BOOST_CHECK_EQUAL(qm_p.getLambdaO(state),0.0);
  BOOST_CHECK_EQUAL(qm_p.getReorg12(state),0.0);
  BOOST_CHECK_EQUAL(qm_p.getReorg21(state),0.0);
  BOOST_CHECK_EQUAL(qm_p.getReorg12_x(state),0.0);
  BOOST_CHECK_EQUAL(qm_p.getReorg21_x(state),0.0);
  BOOST_CHECK_EQUAL(qm_p.getRate12(state),0.0);
  BOOST_CHECK_EQUAL(qm_p.getRate21(state),0.0);
  BOOST_CHECK_EQUAL(qm_p.getJeff2(state),0.0);
  BOOST_CHECK_EQUAL(qm_p.getdE12(state),0.0);
  
  state = 1;
  BOOST_CHECK_EQUAL(qm_p.getLambdaO(state),0.0);
  BOOST_CHECK_EQUAL(qm_p.getReorg12(state),0.0);
  BOOST_CHECK_EQUAL(qm_p.getReorg21(state),0.0);
  BOOST_CHECK_EQUAL(qm_p.getReorg12_x(state),0.0);
  BOOST_CHECK_EQUAL(qm_p.getReorg21_x(state),0.0);
  BOOST_CHECK_EQUAL(qm_p.getRate12(state),0.0);
  BOOST_CHECK_EQUAL(qm_p.getRate21(state),0.0);
  BOOST_CHECK_EQUAL(qm_p.getJeff2(state),0.0);
  BOOST_CHECK_EQUAL(qm_p.getdE12(state),0.0);

  state = 2;
  BOOST_CHECK_EQUAL(qm_p.getLambdaO(state),0.0);
  BOOST_CHECK_EQUAL(qm_p.getReorg12(state),0.0);
  BOOST_CHECK_EQUAL(qm_p.getReorg21(state),0.0);
  BOOST_CHECK_EQUAL(qm_p.getReorg12_x(state),0.0);
  BOOST_CHECK_EQUAL(qm_p.getReorg21_x(state),0.0);
  BOOST_CHECK_EQUAL(qm_p.getRate12(state),0.0);
  BOOST_CHECK_EQUAL(qm_p.getRate21(state),0.0);
  BOOST_CHECK_EQUAL(qm_p.getJeff2(state),0.0);
  BOOST_CHECK_EQUAL(qm_p.getdE12(state),0.0);
  
  state = 3;
  BOOST_CHECK_EQUAL(qm_p.getLambdaO(state),0.0);
  BOOST_CHECK_EQUAL(qm_p.getReorg12(state),0.0);
  BOOST_CHECK_EQUAL(qm_p.getReorg21(state),0.0);
  BOOST_CHECK_EQUAL(qm_p.getReorg12_x(state),0.0);
  BOOST_CHECK_EQUAL(qm_p.getReorg21_x(state),0.0);
  BOOST_CHECK_EQUAL(qm_p.getRate12(state),0.0);
  BOOST_CHECK_EQUAL(qm_p.getRate21(state),0.0);
  BOOST_CHECK_EQUAL(qm_p.getJeff2(state),0.0);
  BOOST_CHECK_EQUAL(qm_p.getdE12(state),0.0);
}

BOOST_AUTO_TEST_SUITE_END()
