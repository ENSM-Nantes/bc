#ifndef SOLVER_HPP
#define SOLVER_HPP

#include <iostream>
#include <Eigen/Dense>
#include "ShipGlobalParams.hpp"
#include "Ship.hpp"
#include "Time.hpp"
#include "Wind.hpp"

#define VECTOR_SIZE_DIFF_EQ (6)

class Solver
{
public:

  /*Constructor*/
  Solver();
  Solver(Ship* aShip);
  
  /*Process*/
  int Init(Ship* aShip);
  void Run(sTime& aTime, Eigen::Vector3d aEta, Eigen::Vector3d aMu, float aColX, float aColY, float aColN, Wind* aWind);
  
  /*Getter*/
  Eigen::Vector3d getEta(void) const;
  Eigen::Vector3d getMu(void) const;
  
  /*Setter*/
  void SetDeltaT(double aDt);
  
private:

  Eigen::VectorXd DiffEq(const Eigen::VectorXd& aVectEtaMu);
  void SolveRk4(Eigen::Vector3d aEta, Eigen::Vector3d aMu);
  void SolveRoll(void);
  void SetDataCollision(float aColX, float aColY, float aColN);
  void SetWindDrag(float aAxialDrag, float aLateralDrag);
  
  
  Eigen::Vector3d mT; //Result Force X, Y, Z
  double mDt; //Time delta
  Eigen::Vector3d mEta; //Pos vector
  Eigen::Vector3d mMu; //Speed vector
  Eigen::Vector3d mTCol; //Collision Force Vector
  Eigen::Vector3d mTWindDrag; //Wind Draft Force Vector
  
  Ship* mShip;  
};

#endif
