/*
 *  Copyright (C) 1997-2013 JDERobot Developers Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, see <http://www.gnu.org/licenses/>.
 *
 *  Authors : Borja Menéndez <borjamonserrano@gmail.com>
 *
 */

#include "poserightelbow.h"

#define RADTODEG 57.29582790

namespace gazebo {
    GZ_REGISTER_MODEL_PLUGIN(PoseRightElbow)

    PoseRightElbow::PoseRightElbow () {
        pthread_mutex_init(&this->mutex_rightelbowencoders, NULL);
        pthread_mutex_init(&this->mutex_rightelbowmotors, NULL);
        this->count = 0;
        this->cycle = 50;
        this->cfgfile_rightelbow = std::string("--Ice.Config=poserightelbow.cfg");
        
        this->rightelbow.motorsparams.maxPan = 1.57;
        this->rightelbow.motorsparams.minPan = -1.57;          
        this->rightelbow.motorsparams.maxTilt = 0.5;
        this->rightelbow.motorsparams.minTilt = -0.5;

        std::cout << "Constructor PoseRightElbow" << std::endl;
    }

    void PoseRightElbow::Load ( physics::ModelPtr _model, sdf::ElementPtr _sdf ) {
        // LOAD CAMERA LEFT
        if (!_sdf->HasElement("joint_pose3dencodersrightelbow_yaw"))
            gzerr << "pose3dencodersrightelbow plugin missing <joint_pose3dencodersrightelbow_yaw> element\n";
        if (!_sdf->HasElement("joint_pose3dencodersrightelbow_roll"))
            gzerr << "pose3dencodersrightelbow plugin missing <joint_pose3dencodersrightelbow_roll> element\n";

        this->rightelbow.joint_pan = _model->GetJoint("relbow_yaw");
        this->rightelbow.joint_roll = _model->GetJoint("relbow_roll");

        if (!this->rightelbow.joint_pan)
            gzerr << "Unable to find joint_pose3dencodersrightelbow_yaw["
                << _sdf->GetElement("joint_pose3dencodersrightelbow_yaw")->GetValueString() << "]\n"; 
        if (!this->rightelbow.joint_roll)
            gzerr << "Unable to find joint_pose3dencodersrightelbow_roll["
                << _sdf->GetElement("joint_pose3dencodersrightelbow_roll")->GetValueString() << "]\n";
                
        this->rightelbow.link_pan = _model->GetLink("rightelbow_yaw");
        this->rightelbow.link_roll = _model->GetLink("right_lower_arm");

        //LOAD TORQUE        
        if (_sdf->HasElement("torque"))
            this->stiffness = _sdf->GetElement("torque")->GetValueDouble();
        else {
            gzwarn << "No torque value set for the rigth elbow.\n";
            this->stiffness = 5.0;
        }

        //LOAD POSE3DMOTORS
        this->updateConnection = event::Events::ConnectWorldUpdateBegin(
                                    boost::bind(&PoseRightElbow::OnUpdate, this));
    }

    void PoseRightElbow::Init () {}

    void PoseRightElbow::OnUpdate () {
        long totalb, totala, diff;
        struct timeval a, b;

        gettimeofday(&a, NULL);
        totala = a.tv_sec * 1000000 + a.tv_usec;

        if (this->count == 0) {
            this->count++;
            pthread_t thr_ice;
            pthread_create(&thr_ice, NULL, &thread_RightElbowICE, (void*) this);
        }

        //          ----------ENCODERS----------
        //GET pose3dencoders data from the right elbow (PAN&TILT)
//        this->rightelbow.encoder.pan = this->rightelbow.rightelbow_link_pan->GetRelativePose().rot.GetAsEuler().z;    
//        this->rightelbow.encoder.roll = this->rightelbow.rightelbow_link_roll->GetRelativePose().rot.GetAsEuler().y;
        
        this->rightelbow.encoders.pan = - this->rightelbow.joint_pan->GetAngle(0).Radian();
        this->rightelbow.encoders.roll = - this->rightelbow.joint_roll->GetAngle(0).Radian();

        //          ----------MOTORS----------
        if (this->rightelbow.motorsdata.pan >= 0) {
            if (this->rightelbow.encoders.pan < this->rightelbow.motorsdata.pan) {
                this->rightelbow.joint_pan->SetVelocity(0, -0.1);
                this->rightelbow.joint_pan->SetMaxForce(0, this->stiffness);
                //std::cout << "AQUI" << std::endl;
            } else {
                this->rightelbow.joint_pan->SetVelocity(0, 0.1);
                this->rightelbow.joint_pan->SetMaxForce(0, this->stiffness);
            }
        } else {
            if (this->rightelbow.encoders.pan > this->rightelbow.motorsdata.pan) {
                this->rightelbow.joint_pan->SetVelocity(0, 0.1);
                this->rightelbow.joint_pan->SetMaxForce(0, this->stiffness);
                //std::cout << "AQUI" << std::endl;
            } else {
                this->rightelbow.joint_pan->SetVelocity(0, -0.1);
                this->rightelbow.joint_pan->SetMaxForce(0, this->stiffness);
            }            
        }
        
        if (this->rightelbow.motorsdata.roll >= 0) {
            if (this->rightelbow.encoders.roll < this->rightelbow.motorsdata.roll) {
                this->rightelbow.joint_roll->SetVelocity(0, -0.1);
                this->rightelbow.joint_roll->SetMaxForce(0, this->stiffness);
            } else {
                this->rightelbow.joint_roll->SetVelocity(0, 0.1);
                this->rightelbow.joint_roll->SetMaxForce(0, this->stiffness);
            }
        } else {
            if (this->rightelbow.encoders.roll > this->rightelbow.motorsdata.roll) {
                this->rightelbow.joint_roll->SetVelocity(0, 0.1);
                this->rightelbow.joint_roll->SetMaxForce(0, this->stiffness);
            } else {
                this->rightelbow.joint_roll->SetVelocity(0, -0.1);
                this->rightelbow.joint_roll->SetMaxForce(0, this->stiffness);
            }
        }

        gettimeofday(&b, NULL);
        totalb = b.tv_sec * 1000000 + b.tv_usec;

        diff = (totalb - totala) / 1000;
        diff = cycle - diff;

        if (diff < 10)
            diff = 10;

        //usleep(diff*1000);
        sleep(diff / 1000);
    }
    
    class Pose3DEncoders : virtual public jderobot::Pose3DEncoders {
    public:

        Pose3DEncoders ( gazebo::PoseRightElbow* pose ) : pose3DEncodersData ( new jderobot::Pose3DEncodersData() ) {
            this->pose = pose;
        }

        virtual ~Pose3DEncoders () {}

        virtual jderobot::Pose3DEncodersDataPtr getPose3DEncodersData ( const Ice::Current& ) {
            pthread_mutex_lock(&pose->mutex_rightelbowencoders);
            
            pose3DEncodersData->x = pose->rightelbow.encoders.x;
            pose3DEncodersData->y = pose->rightelbow.encoders.y;
            pose3DEncodersData->z = pose->rightelbow.encoders.z;
            pose3DEncodersData->pan = pose->rightelbow.encoders.pan;
            pose3DEncodersData->tilt = pose->rightelbow.encoders.tilt;
            pose3DEncodersData->roll = pose->rightelbow.encoders.roll;
            pose3DEncodersData->clock = pose->rightelbow.encoders.clock;
            pose3DEncodersData->maxPan = pose->rightelbow.encoders.maxPan;
            pose3DEncodersData->minPan = pose->rightelbow.encoders.minPan;
            pose3DEncodersData->maxTilt = pose->rightelbow.encoders.maxTilt;
            pose3DEncodersData->minTilt = pose->rightelbow.encoders.minTilt;
            
            pthread_mutex_unlock(&pose->mutex_rightelbowencoders);

            return pose3DEncodersData;
        }

        gazebo::PoseRightElbow* pose;

    private:
        jderobot::Pose3DEncodersDataPtr pose3DEncodersData;
    };

    class Pose3DMotors : virtual public jderobot::Pose3DMotors {
    public:

        Pose3DMotors (gazebo::PoseRightElbow* pose) : pose3DMotorsData ( new jderobot::Pose3DMotorsData() ) {
            this->pose = pose;
        }

        virtual ~Pose3DMotors() {}

        virtual jderobot::Pose3DMotorsDataPtr getPose3DMotorsData ( const Ice::Current& ) {
            pthread_mutex_lock(&pose->mutex_rightelbowmotors);
            
            pose3DMotorsData->x = pose->rightelbow.motorsdata.x;
            pose3DMotorsData->y = pose->rightelbow.motorsdata.y;
            pose3DMotorsData->z = pose->rightelbow.motorsdata.z;
            pose3DMotorsData->pan = pose->rightelbow.motorsdata.pan;
            pose3DMotorsData->tilt = pose->rightelbow.motorsdata.tilt;
            pose3DMotorsData->roll = pose->rightelbow.motorsdata.roll;
            pose3DMotorsData->panSpeed = pose->rightelbow.motorsdata.panSpeed;
            pose3DMotorsData->tiltSpeed = pose->rightelbow.motorsdata.tiltSpeed;
            
            pthread_mutex_unlock(&pose->mutex_rightelbowmotors);

            return pose3DMotorsData;
        }

        virtual jderobot::Pose3DMotorsParamsPtr getPose3DMotorsParams ( const Ice::Current& ) {
            pthread_mutex_lock(&pose->mutex_rightelbowmotors);
            
            pose3DMotorsParams->maxPan = pose->rightelbow.motorsparams.maxPan;
            pose3DMotorsParams->minPan = pose->rightelbow.motorsparams.minPan;
            pose3DMotorsParams->maxTilt = pose->rightelbow.motorsparams.maxTilt;
            pose3DMotorsParams->minTilt = pose->rightelbow.motorsparams.minTilt;
            pose3DMotorsParams->maxPanSpeed = pose->rightelbow.motorsparams.maxPanSpeed;
            pose3DMotorsParams->maxTiltSpeed = pose->rightelbow.motorsparams.maxTiltSpeed;
            
            pthread_mutex_unlock(&pose->mutex_rightelbowmotors);
            
            return pose3DMotorsParams;
        }

        virtual Ice::Int setPose3DMotorsData ( const jderobot::Pose3DMotorsDataPtr & data, const Ice::Current& ) {
            pthread_mutex_lock(&pose->mutex_rightelbowmotors);
            
            pose->rightelbow.motorsdata.x = data->x;
            pose->rightelbow.motorsdata.y = data->y;
            pose->rightelbow.motorsdata.z = data->z;
            pose->rightelbow.motorsdata.pan = data->pan;
            pose->rightelbow.motorsdata.tilt = data->tilt;
            pose->rightelbow.motorsdata.roll = data->roll;
            pose->rightelbow.motorsdata.panSpeed = data->panSpeed;
            pose->rightelbow.motorsdata.tiltSpeed = data->tiltSpeed;
            
            pthread_mutex_unlock(&pose->mutex_rightelbowmotors);
        }

        gazebo::PoseRightElbow* pose;

    private:
        jderobot::Pose3DMotorsDataPtr pose3DMotorsData;
        jderobot::Pose3DMotorsParamsPtr pose3DMotorsParams;
    };

    void* thread_RightElbowICE ( void* v ) {

        gazebo::PoseRightElbow* rightelbow = (gazebo::PoseRightElbow*)v;
        char* name = (char*) rightelbow->cfgfile_rightelbow.c_str();
        Ice::CommunicatorPtr ic;
        int argc = 1;
        Ice::PropertiesPtr prop;
        char* argv[] = {name};

        try {
            ic = Ice::initialize(argc, argv);

            prop = ic->getProperties();
            std::string EndpointsEncoders = prop->getProperty("PoseRightElbowEncoders.Endpoints");
            std::cout << "PoseRightElbowEncoders Endpoints > " << EndpointsEncoders << std::endl;
            std::string EndpointsMotors = prop->getProperty("PoseRightElbowMotors.Endpoints");
            std::cout << "PoseRightElbowMotors Endpoints > " << EndpointsMotors << std::endl;

            Ice::ObjectAdapterPtr AdapterEncoders =
                    ic->createObjectAdapterWithEndpoints("AdapterRightElbowEncoders", EndpointsEncoders);
            Ice::ObjectAdapterPtr AdapterMotors =
                    ic->createObjectAdapterWithEndpoints("AdapterRightElbowMotors", EndpointsMotors);

            Ice::ObjectPtr encoders = new Pose3DEncoders(rightelbow);
            Ice::ObjectPtr motors = new Pose3DMotors(rightelbow);

            AdapterEncoders->add(encoders, ic->stringToIdentity("RightElbowEncoders"));
            AdapterMotors->add(motors, ic->stringToIdentity("RightElbowMotors"));

            AdapterEncoders->activate();
            AdapterMotors->activate();

            ic->waitForShutdown();
        } catch (const Ice::Exception& e) {
            std::cerr << e << std::endl;
        } catch (const char* msg) {
            std::cerr << msg << std::endl;
        }
        if (ic) {
            try {
                ic->destroy();
            } catch (const Ice::Exception& e) {
                std::cerr << e << std::endl;
            }
        }
    }
}