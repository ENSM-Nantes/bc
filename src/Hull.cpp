#include "Hull.hpp"
#include <cmath>

Hull::Hull(void)
{
  Init(0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,false);
  mT << 0, 0, 0;
  mKh = 0;
}

void Hull::Init(double aXp0, double aXpVV, double aXpVR, double aXpRR, double aXpVVVV, double aYpV, double aYpR, double aYpVVV,
		double aYpVVR, double aYpVRR, double aYpRRR, double aNpV, double aNpR, double aNpVVV, double aNpVVR,
		double aNpVRR, double aNpRRR, double aKpG, double aKpB, double aKpR, double aKpBBG, double aKpBRG,
		double aKpRRG, double aKpBBB, double aKpBBR, double aKpBRR, double aKpRRR, bool aInvertRoll)
{
  mXp0 = aXp0;
  mXpVV = aXpVV;
  mXpVR = aXpVR; 
  mXpRR = aXpRR;
  mXpVVVV = aXpVVVV;
  mYpV = aYpV;
  mYpR = aYpR;
  mYpVVV = aYpVVV;
  mYpVVR = aYpVVR;
  mYpVRR = aYpVRR;
  mYpRRR = aYpRRR;
  mNpV = aNpV;
  mNpR = aNpR;
  mNpVVV = aNpVVV;
  mNpVVR = aNpVVR;
  mNpVRR = aNpVRR;
  mNpRRR = aNpRRR;
  mKpG = aKpG;
  mKpB = aKpB;
  mKpR = aKpR;
  mKpBBG = aKpBBG;
  mKpBRG = aKpBRG;
  mKpRRG = aKpRRG;
  mKpBBB = aKpBBB;
  mKpBBR = aKpBBR;
  mKpBRR = aKpBRR;
  mKpRRR = aKpRRR;
  mInvertRoll = aInvertRoll;
}

void Hull::PrintParams(void)
{
  std::cout << "::::::Hull Parameters::::::" << std::endl;
  std::cout << "Xp0 : " << mXp0 << std::endl;
  std::cout << "XpVV : " << mXpVV << std::endl;
  std::cout << "XpVR : " << mXpVR << std::endl; 
  std::cout << "XpRR : " << mXpRR << std::endl;
  std::cout << "XpVVVV : " << mXpVVVV << std::endl;
  std::cout << "YpV : " << mYpV << std::endl;
  std::cout << "YpR : " << mYpR << std::endl;
  std::cout << "YpVVV : " << mYpVVV << std::endl;
  std::cout << "YpVVR : " << mYpVVR << std::endl;
  std::cout << "YpVRR : " << mYpVRR << std::endl;
  std::cout << "YpRRR : " << mYpRRR << std::endl;
  std::cout << "NpV : " << mNpV << std::endl;
  std::cout << "NpR : " << mNpR << std::endl;
  std::cout << "NpVVV : " << mNpVVV << std::endl;
  std::cout << "NpVVR : " << mNpVVR << std::endl;
  std::cout << "NpVRR : " << mNpVRR << std::endl;
  std::cout << "NpRRR : " << mNpRRR << std::endl;
  std::cout << "KpG : " << mKpG << std::endl;
  std::cout << "KpB : " << mKpB << std::endl;
  std::cout << "KpR : " << mKpR << std::endl;
  std::cout << "KpBBG : " << mKpBBG << std::endl;
  std::cout << "KpBRG : " << mKpBRG << std::endl;
  std::cout << "KpRRG : " << mKpRRG << std::endl;
  std::cout << "KpBB : " << mKpBBB << std::endl;
  std::cout << "KpBBR : " << mKpBBR << std::endl;
  std::cout << "KpBRR : " << mKpBRR << std::endl;
  std::cout << "KpRRR : " << mKpRRR << std::endl;
  std::cout << "::::::::::::" << std::endl;
}


void Hull::ComputeT(const Eigen::Vector3d& aMu, const double aRho, const sGeoParams& aGeo, double aRollAngle)
{
  double u = 0, kf = 0, km = 0, kr = 0, vp = 0, beta = 0;
  double rp = 0, xph = 0, yph = 0, nph = 0;

  // ***** H. Yasukawa and Y. Yoshimura 2015 *******
  u = pow((pow(aMu[0], 2) + pow(aMu[1], 2)), 0.5);
  kf = 0.5 * aRho * aGeo.lPP * aGeo.d * pow(u , 2);
  km = 0.5 * aRho * pow(aGeo.lPP, 2) * aGeo.d * pow(u , 2);

  if(0 != aMu[0])
    beta = atan(-(aMu[1])/aMu[0]);
  else
    beta = 0;
  
  if(0 !=  u)
    {
      vp = aMu[1] / u;
      rp = aMu[2] * aGeo.lPP/u;
    }
  else
    {
      vp = 0;
      rp = 0;
    }

  /*Equation (6) and (7)*/
  xph = -mXp0 + (mXpVV * pow(vp, 2)) + (mXpVR * vp * rp) + (mXpRR * pow(rp, 2)) + (mXpVVVV * pow(vp, 4));
  
  yph = (mYpV * vp) + (mYpR * rp) + (mYpVVV * pow(vp, 3)) + (mYpVVR * pow(vp, 2) * rp) + (mYpVRR * pow(rp, 2) * vp) + (mYpRRR * pow(rp, 3));

  nph = (mNpV * vp) + (mNpR * rp) + (mNpVVV * pow(vp, 3)) + (mNpVVR * pow(vp, 2) * rp) + (mNpVRR * pow(rp, 2) * vp) + (mNpRRR * pow(rp, 3));

  mT << kf*xph, kf*yph, km*nph;


  //***** 4-DOF MathematicalModel for manoeuvring simulation including roll motion
  kr = 0.5 * aRho  * aGeo.lPP * pow(aGeo.d, 2) * pow(u , 2);
  mKh = kr * ((mKpG * aRollAngle) + (mKpB * beta) + (mKpR * rp) +
	      (mKpBBG * (beta*beta) * aRollAngle) + (mKpBRG * beta * rp * aRollAngle) + (mKpRRG * (rp*rp) * aRollAngle) +
	      (mKpBBB * pow(beta, 3)) + (mKpBBR * (beta*beta) * rp) + (mKpBRR * beta * (rp*rp)) + (mKpRRR * pow(rp, 3))
	      );
  

  
  //std::cout << "****Hull mT :" << mT << std::endl; 
  //************
}

Eigen::Vector3d& Hull::getT(void){return mT;}
double Hull::getKh(void){return mKh;}
bool Hull::getInvertRoll(void) { return mInvertRoll; }
