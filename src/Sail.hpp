#ifndef SAIL_HPP
#define SAIL_HPP

#include <string>
#include <vector>
#include <netcdf.h>
#include <Eigen/Dense>
#include "irrlicht.h"

#define TOTAL_SAIL_DIM_COUNT (5)
#define SAILS_MAX (4)
#define ROTOR_TIME_TO_MAX_SPEED (30)
#define ROTOR_MAX_SPEED (3.12)

/*Suction sail penalty applied to the polar force (1 = optimal settings)*/
#define SUCTION_OFF_FACTOR (0.3f)        //Suction off : the thick body stalls
#define SUCTION_FLAP_ZERO_FACTOR (0.2f)  //Flap centred : no trailing edge, little lift
#define SUCTION_FLAP_MAX (35.0f)
#define SUCTION_AOA_OPT (20.0f)          //Incidence of maximum force
#define SUCTION_AOA_MAX (50.0f)          //Incidence where the force is lost
#define SUCTION_DOWNWIND_DEG (150.0f)    //Downwind the sail only works as drag

class Sail
{
 public:

  Sail();
  Sail(const std::string aPolarFile, std::string aVarNameX, std::string aVarNameY);
  ~Sail();

  /*Sails*/
  void Init(int aSailsCount, std::string aSailsType, std::string aSailsSize, float (*aSailsPos)[3]);
  void SetMeshScene(irr::scene::IMeshSceneNode *aMeshScene);
  irr::scene::IMeshSceneNode* GetMeshScene(unsigned char aIndex);
  unsigned char GetCount(void);
  std::string GetType(void);
  std::string GetSize(void);
  float (*GetPos(void))[3];
  void PrintParams(void);
  void UpdateMesh(irr::IrrlichtDevice *aDev);
  /*Rotor*/
  void SetOnOff(bool aOnOff);
  void SetRotDirection(std::string aRotDirection);
  float GetRotSpeed(void);
  bool GetOnOff(void);
  int GetRotDirection(void);
  /*Suction sail*/
  void SetSuction(bool aSuctionOn, float aBodyAngle, float aFlapAngle);
  float GetSuctionEfficiency(double aAwaDeg);
  
  /*Polar file*/
  int OpenPolar(const std::string aPolarFile, std::string aVarNameX, std::string aVarNameY);
  int InitPolar(std::string aSpeedWaterVarName, std::string aWindSpeedVarName, std::string aWindAngleVarName);
  float GetForce(char aAxe, float aStwValue, float aTwsValue, float aTwaValue);

  /*Ship*/
  void ComputeT(void);
  void SetSTW(double aSpeedThroughWater);
  void SetWind(double aTrueWindSpeed, double aApparentWindDir);
  Eigen::Vector3d& getT(void);
  
 private:

  //Mesh Sails
  irr::scene::IMeshSceneNode* mSailsScene[SAILS_MAX];
  int mCountMeshScene;

  //Rotor
  bool mOnOff;
  bool mWaitForStop;
  bool mWaitForStart;
  int mRotDirection;
  float mRotSpeed;

  //Suction sail
  bool mSuctionOn;
  float mBodyAngle;
  float mFlapAngle;
  
  //Sail management
  int mSailsCount;
  std::string mSailsType;
  std::string mSailsSize;
  float mSailsPos[SAILS_MAX][3];

  //Compute force and manage nc file
  int mIdPolarFile;

  int mSailVarX;
  int mDimCountX;
  int mSailVarY;
  int mDimCountY;

  std::vector<float> mStw;
  std::vector<float> mTws;
  std::vector<float> mTwa;

  double mSpeedThroughWater;
  double mTrueWindSpeed;
  double mApparentWindDir;
  
  Eigen::Vector3d mT; //Sail force generated 
};




#endif
