/*
 * OpenNi.cpp
 *
 *  Created on: 24.06.2011
 *      Author: Matthias
 */

#include "OpenNi.h"

#include <XnOpenNI.h>
#include <XnCodecIDs.h>
#include <XnCppWrapper.h>
#include <XnTypes.h>
#include <XnPlatform.h>

#include "Application.h"
#include "Player.h"
#include "TGString.h"
#include "Calculation.h"

#define SAMPLE_XML_PATH "configs/OpenNi.xml"

#define CHECK_RC(nRetVal, what) printf("%s : %s\n", what, xnGetStatusString(nRetVal));

xn::Context        g_Context;
xn::UserGenerator  g_UserGenerator;

xn::ImageGenerator g_Image;
xn::DepthGenerator g_Depth;

XnCallbackHandle h;
XnCallbackHandle hCalib;
XnCallbackHandle hPose;

void XN_CALLBACK_TYPE NewUser(xn::UserGenerator& generator, XnUserID user, void* pCookie)
{
   printf("New user identified: %d\n", user);
   Application::get()->playerCallback.playerRecognized(user);
   g_UserGenerator.GetPoseDetectionCap().StartPoseDetection("Psi", user);
}
//---------------------------------------------------------------------------

void XN_CALLBACK_TYPE LostUser(xn::UserGenerator& generator, XnUserID user, void* pCookie)
{
   printf("User %d lost\n", user);
   Application::get()->playerCallback.playerLost(user);
}
//---------------------------------------------------------------------------

void XN_CALLBACK_TYPE CalibrationStart(xn::SkeletonCapability& skeleton, XnUserID user, void* pCookie)
{
   printf("Calibration start for user %d\n", user);
   Application::get()->playerCallback.calibrationStart(user);
}
//---------------------------------------------------------------------------

void XN_CALLBACK_TYPE CalibrationEnd(xn::SkeletonCapability& skeleton, XnUserID user, XnBool bSuccess, void* pCookie)
{
   printf("Calibration complete for user %d: %s\n", user, bSuccess?"Success":"Failure");
   if (bSuccess)
   {
      skeleton.StartTracking(user);
      Application::get()->playerCallback.playerCalibrated(user);
   }
   else
   {
      g_UserGenerator.GetPoseDetectionCap().StartPoseDetection("Psi", user);
      Application::get()->playerCallback.calibrationFailed(user);
   }
}
//---------------------------------------------------------------------------

void XN_CALLBACK_TYPE PoseDetected(xn::PoseDetectionCapability& poseDetection, const XnChar* strPose, XnUserID nid, void* pCookie)
{
   printf("Pose '%s' detected for user %d\n", strPose, nid);
   g_UserGenerator.GetSkeletonCap().RequestCalibration(nid, FALSE);
   g_UserGenerator.GetPoseDetectionCap().StopPoseDetection(nid);
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

void OpenNiPlayer::changeForDisplay()
{
   for(int i=0; i < P_SIZE; i++)
   {
      pointList.at(i)->x = pointList.at(i)->x * 0.3 + Application::x_res/2;
      pointList.at(i)->y = pointList.at(i)->y * 0.3 + Application::y_res/2;
   }
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------

OpenNi::OpenNi()
{
   init_ok = false;
   init();
}
//---------------------------------------------------------------------------

OpenNi::~OpenNi()
{
   if(init_ok) g_Context.Shutdown();
}
//---------------------------------------------------------------------------

void OpenNi::init()
{
   if(init_ok) return;

   XnStatus rc = XN_STATUS_OK;
   xn::EnumerationErrors errors;

   rc = g_Context.Init();
   CHECK_RC(rc, "Context.init");

   rc = g_Context.InitFromXmlFile(SAMPLE_XML_PATH);
   CHECK_RC(rc, "InitFromXml");

   rc = g_Image.Create(g_Context);
   CHECK_RC(rc, "CreateImageGenerator");
   if(rc != XN_STATUS_OK) return;
   rc = g_Depth.Create(g_Context);
   CHECK_RC(rc, "CreateDepthGenerator");
   if(rc != XN_STATUS_OK) return;

   rc = g_Context.FindExistingNode(XN_NODE_TYPE_DEPTH, g_Depth);
   CHECK_RC(rc, "Find depth generator");
   if(rc != XN_STATUS_OK) return;
   rc = g_Context.FindExistingNode(XN_NODE_TYPE_USER, g_UserGenerator);
   CHECK_RC(rc, "Find user generator");

   if(rc != XN_STATUS_OK)
   {
      rc = g_UserGenerator.Create(g_Context);
      CHECK_RC(rc, "create UserGenerator");
      if(rc != XN_STATUS_OK) return;
   }

   rc = g_UserGenerator.RegisterUserCallbacks(NewUser, LostUser, this, h);
   CHECK_RC(rc, "create RegisterUserCallbacks");
   if(rc != XN_STATUS_OK) return;
   rc = g_UserGenerator.GetSkeletonCap().SetSkeletonProfile(XN_SKEL_PROFILE_ALL);
   CHECK_RC(rc, "SetSkeletonProfile");
   if(rc != XN_STATUS_OK) return;

   rc = g_UserGenerator.GetSkeletonCap().RegisterCalibrationCallbacks(&CalibrationStart, &CalibrationEnd, NULL, hCalib);
   CHECK_RC(rc, "RegisterCalibrationCallbacks");
   if(rc != XN_STATUS_OK) return;
   rc = g_UserGenerator.GetPoseDetectionCap().RegisterToPoseCallbacks(&PoseDetected, NULL, NULL, hPose);
   CHECK_RC(rc, "RegisterToPoseCallbacks");
   if(rc != XN_STATUS_OK) return;

   rc = g_Context.StartGeneratingAll();
   CHECK_RC(rc, "StartGeneratingAll");
   if(rc != XN_STATUS_OK) return;

   timepast = 42;
   init_ok = true;
}
//---------------------------------------------------------------------------

void OpenNi::update(float timediff)
{
   if(init_ok)
   {
      timepast += timediff;

      g_Context.WaitAndUpdateAll();
      if(timepast > 42)
      {
         pixels = (unsigned short*)g_Image.GetImageMap();
         depth  = (unsigned short*)g_Depth.GetDepthMap();
      }

   }
 //  else init();
}
//---------------------------------------------------------------------------

int OpenNi::getAnzPlayer()
{
   if(init_ok) return g_UserGenerator.GetNumberOfUsers();
   else return 0;
}
//---------------------------------------------------------------------------

OpenNiPlayer OpenNi::getPlayer(int nr)
{
   XnUserID aUsers[15];
   XnUInt16 nUsers = 15;

   OpenNiPlayer p;
   if(!init_ok) return p;

   g_UserGenerator.GetUsers(aUsers, nUsers);
   if(g_UserGenerator.GetSkeletonCap().IsTracking(nr))
   {
      try
      {
         for(int i=0; i < P_SIZE; i++)
         {
            *(p.pointList.at(i)) = getPlayerPart(nr, i);
         }
         p.calibrated = true;
      }
      catch(...){}
   }
   return p;
}
//---------------------------------------------------------------------------

OpenNiPoint OpenNi::getPlayerPart(int nr, int part, bool projective)
{
   OpenNiPoint p;
   if(!init_ok) return p;

   if(g_UserGenerator.GetSkeletonCap().IsTracking(nr))
   {
      XnSkeletonJointPosition pos;
      xn::SkeletonCapability s = g_UserGenerator.GetSkeletonCap();

      switch(part)
      {
      case P_HEAD:      s.GetSkeletonJointPosition(nr, XN_SKEL_HEAD,  pos); break;
      case P_NECK:      s.GetSkeletonJointPosition(nr, XN_SKEL_NECK,  pos); break;
      case P_TORSO:     s.GetSkeletonJointPosition(nr, XN_SKEL_TORSO, pos); break;
      case P_WAIST:     s.GetSkeletonJointPosition(nr, XN_SKEL_WAIST, pos); break;
      case P_LCOLLAR:   s.GetSkeletonJointPosition(nr, XN_SKEL_LEFT_COLLAR,     pos); break;
      case P_LSHOULDER: s.GetSkeletonJointPosition(nr, XN_SKEL_LEFT_SHOULDER,   pos); break;
      case P_LELBOW:    s.GetSkeletonJointPosition(nr, XN_SKEL_LEFT_ELBOW,      pos); break;
      case P_LWRIST:    s.GetSkeletonJointPosition(nr, XN_SKEL_LEFT_WRIST,      pos); break;
      case P_LHAND:     s.GetSkeletonJointPosition(nr, XN_SKEL_LEFT_HAND,       pos); break;
      case P_LFINGER:   s.GetSkeletonJointPosition(nr, XN_SKEL_LEFT_FINGERTIP,  pos); break;
      case P_RCOLLAR:   s.GetSkeletonJointPosition(nr, XN_SKEL_RIGHT_COLLAR,    pos); break;
      case P_RSHOULDER: s.GetSkeletonJointPosition(nr, XN_SKEL_RIGHT_SHOULDER,  pos); break;
      case P_RELBOW:    s.GetSkeletonJointPosition(nr, XN_SKEL_RIGHT_ELBOW,     pos); break;
      case P_RWRIST:    s.GetSkeletonJointPosition(nr, XN_SKEL_RIGHT_WRIST,     pos); break;
      case P_RHAND:     s.GetSkeletonJointPosition(nr, XN_SKEL_RIGHT_HAND,      pos); break;
      case P_RFINGER:   s.GetSkeletonJointPosition(nr, XN_SKEL_RIGHT_FINGERTIP, pos); break;
      case P_LHIP:      s.GetSkeletonJointPosition(nr, XN_SKEL_LEFT_HIP,        pos); break;
      case P_LKNEE:     s.GetSkeletonJointPosition(nr, XN_SKEL_LEFT_KNEE,       pos); break;
      case P_LANKLE:    s.GetSkeletonJointPosition(nr, XN_SKEL_LEFT_ANKLE,      pos); break;
      case P_LFOOT:     s.GetSkeletonJointPosition(nr, XN_SKEL_LEFT_FOOT,       pos); break;
      case P_RHIP:      s.GetSkeletonJointPosition(nr, XN_SKEL_RIGHT_HIP,       pos); break;
      case P_RKNEE:     s.GetSkeletonJointPosition(nr, XN_SKEL_RIGHT_KNEE,      pos); break;
      case P_RANKLE:    s.GetSkeletonJointPosition(nr, XN_SKEL_RIGHT_ANKLE,     pos); break;
      case P_RFOOT:     s.GetSkeletonJointPosition(nr, XN_SKEL_RIGHT_FOOT,      pos); break;
      }

      if(!projective)
      {
         p.x = pos.position.X;// * -1.0;
         p.y = pos.position.Y * -1.0;
         p.z = abs(pos.position.Z);
      }
      else
      {
         XnPoint3D pos2[2];
         pos2->X = pos.position.X;
         pos2->Y = pos.position.Y;
         pos2->Z = pos.position.Z;
         g_Depth.ConvertRealWorldToProjective(1, pos2, pos2);
         p.x = pos2->X;
         p.y = pos2->Y;
         p.z = pos2->Z;
      }

   }
   return p;
}
//---------------------------------------------------------------------------

OpenNiPoint OpenNi::getPlayerPart(int nr, int part1, int part2, bool projective)
{
   OpenNiPoint p1 = getPlayerPart(nr, part1, projective);
   OpenNiPoint p2 = getPlayerPart(nr, part2, projective);

   return p1 - p2;
}
//---------------------------------------------------------------------------

unsigned short* OpenNi::getRGBPicture()
{
   if(!init_ok) return NULL;

   return pixels; //(unsigned short*)g_Image.GetImageMap();//
}
//---------------------------------------------------------------------------

unsigned short* OpenNi::getDepthPicture()
{
   if(!init_ok) return NULL;

   return depth;//(unsigned short*)g_Depth.GetDepthMap();//
}
//---------------------------------------------------------------------------

double OpenNi::getWinkelELBOW(int nr, int leftArm)
{
   OpenNiPoint p1, p2;
   if(!init_ok) return 0;

   if(leftArm)
   {
      p1 = getPlayerPart(nr, P_LHAND,     P_LELBOW);
      p2 = getPlayerPart(nr, P_LSHOULDER, P_LELBOW);
   }
   else
   {
      p1 = getPlayerPart(nr, P_RHAND,     P_RELBOW);
      p2 = getPlayerPart(nr, P_RSHOULDER, P_RELBOW);
   }

   return Calculation::getWinkel(p1,p2);
}
//---------------------------------------------------------------------------

double OpenNi::getWinkel(int nr, int pos1, int gelenk, int pos2)
{
   if(!init_ok) return 0;

   OpenNiPoint p1, p2;

   p1 = getPlayerPart(nr, pos1, gelenk);
   p2 = getPlayerPart(nr, pos2, gelenk);

   return Calculation::getWinkel(p1,p2);
}
//---------------------------------------------------------------------------

void OpenNi::drawPlayer(int nr)
{
   if(!init_ok) return;

   CL_FontDescription font_desc;
   font_desc.set_typeface_name("tahoma");
   font_desc.set_height(30);
   CL_Font_System font(Application::get()->getGC(), font_desc);

   CL_Colorf color;
   OpenNiPlayer p = getPlayer(nr);
   if(p.calibrated)
   {
      color = CL_Colorf::grey;

      for(int i=0; i < Application::get()->players.size(); i++)
      {
         if(Application::get()->players.at(i) != NULL)
         {
            if(Application::get()->players.at(i)->getNumber() == nr)
            {
               color = playerColors[i];
            }
         }
      }

      p.changeForDisplay();
      for(int i=0; i < P_SIZE; i++)
      {
         if((p.pointList.at(i)->x != Application::x_res/2) && (p.pointList.at(i)->y != Application::y_res/2)) CL_Draw::circle(Application::get()->getGC(), CL_Pointf(p.pointList.at(i)->x, p.pointList.at(i)->y), 5, color);
      }

      CL_Draw::line(Application::get()->getGC(), p.pointList.at(P_HEAD)->x,  p.pointList.at(P_HEAD)->y,  p.pointList.at(P_NECK)->x, p.pointList.at(P_NECK)->y, color);
      CL_Draw::line(Application::get()->getGC(), p.pointList.at(P_TORSO)->x, p.pointList.at(P_TORSO)->y, p.pointList.at(P_NECK)->x, p.pointList.at(P_NECK)->y, color);

      CL_Draw::line(Application::get()->getGC(), p.pointList.at(P_LSHOULDER)->x, p.pointList.at(P_LSHOULDER)->y, p.pointList.at(P_RSHOULDER)->x, p.pointList.at(P_RSHOULDER)->y, color);

      CL_Draw::line(Application::get()->getGC(), p.pointList.at(P_LELBOW)->x,    p.pointList.at(P_LELBOW)->y,    p.pointList.at(P_LHAND)->x,  p.pointList.at(P_LHAND)->y,  color);
      CL_Draw::line(Application::get()->getGC(), p.pointList.at(P_LSHOULDER)->x, p.pointList.at(P_LSHOULDER)->y, p.pointList.at(P_LELBOW)->x, p.pointList.at(P_LELBOW)->y, color);

      CL_Draw::line(Application::get()->getGC(), p.pointList.at(P_RELBOW)->x,    p.pointList.at(P_RELBOW)->y,    p.pointList.at(P_RHAND)->x,  p.pointList.at(P_RHAND)->y,  color);
      CL_Draw::line(Application::get()->getGC(), p.pointList.at(P_RSHOULDER)->x, p.pointList.at(P_RSHOULDER)->y, p.pointList.at(P_RELBOW)->x, p.pointList.at(P_RELBOW)->y, color);

      CL_Draw::line(Application::get()->getGC(), p.pointList.at(P_LHIP)->x,  p.pointList.at(P_LHIP)->y,  p.pointList.at(P_TORSO)->x, p.pointList.at(P_TORSO)->y, color);
      CL_Draw::line(Application::get()->getGC(), p.pointList.at(P_TORSO)->x, p.pointList.at(P_TORSO)->y, p.pointList.at(P_RHIP)->x,  p.pointList.at(P_RHIP)->y,  color);
      CL_Draw::line(Application::get()->getGC(), p.pointList.at(P_LHIP)->x,  p.pointList.at(P_LHIP)->y,  p.pointList.at(P_RHIP)->x,  p.pointList.at(P_RHIP)->y,  color);
      CL_Draw::line(Application::get()->getGC(), p.pointList.at(P_LHIP)->x,  p.pointList.at(P_LHIP)->y,  p.pointList.at(P_LKNEE)->x, p.pointList.at(P_LKNEE)->y, color);
      CL_Draw::line(Application::get()->getGC(), p.pointList.at(P_RHIP)->x,  p.pointList.at(P_RHIP)->y,  p.pointList.at(P_RKNEE)->x, p.pointList.at(P_RKNEE)->y, color);
      CL_Draw::line(Application::get()->getGC(), p.pointList.at(P_RFOOT)->x, p.pointList.at(P_RFOOT)->y, p.pointList.at(P_RKNEE)->x, p.pointList.at(P_RKNEE)->y, color);
      CL_Draw::line(Application::get()->getGC(), p.pointList.at(P_LFOOT)->x, p.pointList.at(P_LFOOT)->y, p.pointList.at(P_LKNEE)->x, p.pointList.at(P_LKNEE)->y, color);
   }
}
//---------------------------------------------------------------------------

void OpenNi::setPlayerCallback(OpenNiPlayerCallback* callback)
{
	playerCallback = callback;
}
