/* ****************************************************************** **
**    OpenSees - Open System for Earthquake Engineering Simulation    **
**          Pacific Earthquake Engineering Research Center            **
**                                                                    **
**                                                                    **
** (C) Copyright 1999, The Regents of the University of California    **
** All Rights Reserved.                                               **
**                                                                    **
** Commercial use of this program without express permission of the   **
** University of California, Berkeley, is strictly prohibited.  See   **
** file 'COPYRIGHT'  in main directory for information on usage and   **
** redistribution,  and for a DISCLAIMER OF ALL WARRANTIES.           **
**                                                                    **
** Developed by:                                                      **
**   Frank McKenna (fmckenna@ce.berkeley.edu)                         **
**   Gregory L. Fenves (fenves@ce.berkeley.edu)                       **
**   Filip C. Filippou (filippou@ce.berkeley.edu)                     **
**                                                                    **
** ****************************************************************** */

#include <math.h>
#include <IMKPinching.h>
#include <elementAPI.h>
#include <Vector.h>
#include <Channel.h>
#include <OPS_Globals.h>
#include <algorithm>

#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string>
using namespace std;

static int numIMKPinchingMaterials = 0;

void *
OPS_IMKPinching()
{
	if (numIMKPinchingMaterials == 0) {
		numIMKPinchingMaterials++;
		OPS_Error("IMK with Pinched Response - Code by AE_KI (Oct22)\n", 1);
	}

	// Pointer to a uniaxial material that will be returned
	UniaxialMaterial *theMaterial = 0;

	int    iData[1];
	double dData[25];
	int numData = 1;

	if (OPS_GetIntInput(&numData, iData) != 0) {
		opserr << "WARNING invalid uniaxialMaterial IMKPinching tag" << endln;
		return 0;
	}

	numData = 25;


    if (OPS_GetDoubleInput(&numData, dData) != 0) {
        opserr << "Invalid Args want: uniaxialMaterial IMKPinching tag? Ke? ";
        opserr << "posUp_0? posUpc_0? posUu_0? posFy_0? posFcapFy_0? posFresFy_0? ";
        opserr << "negUp_0? negUpc_0? negUu_0? negFy_0? negFcapFy_0? negFresFy_0? ";
        opserr << "LamdaS? LamdaC? LamdaA? LamdaK? Cs? Cc? Ca? Ck? D_pos? D_neg? kappaF? kappaD? ";
        return 0;
    }



	// Parsing was successful, allocate the material
	theMaterial = new IMKPinching(iData[0],
		dData[0],
		dData[1], dData[2], dData[3], dData[4], dData[5], dData[6],
		dData[7], dData[8], dData[9], dData[10], dData[11], dData[12],
		dData[13], dData[14], dData[15], dData[16], dData[17], dData[18], dData[19], dData[20],
		dData[21], dData[22],dData[23], dData[24]);

	if (theMaterial == 0) {
		opserr << "WARNING could not create uniaxialMaterial of type IMKPinching Material\n";
		return 0;
	}

	return theMaterial;
}

IMKPinching::IMKPinching(int tag, double p_Ke,
    double p_posUp_0, double p_posUpc_0, double p_posUu_0, double p_posFy_0, double p_posFcapFy_0, double p_posFresFy_0,
    double p_negUp_0, double p_negUpc_0, double p_negUu_0, double p_negFy_0, double p_negFcapFy_0, double p_negFresFy_0,
    double p_LAMBDA_S, double p_LAMBDA_C, double p_LAMBDA_A, double p_LAMBDA_K, double p_c_S, double p_c_C, double p_c_A, double p_c_K, double p_D_pos, double p_D_neg, double p_kappaF, double p_kappaD)
    : UniaxialMaterial(tag, 0), Ke(p_Ke),
    posUp_0(p_posUp_0), posUpc_0(p_posUpc_0), posUu_0(p_posUu_0), posFy_0(p_posFy_0), posFcapFy_0(p_posFcapFy_0), posFresFy_0(p_posFresFy_0),
    negUp_0(p_negUp_0), negUpc_0(p_negUpc_0), negUu_0(p_negUu_0), negFy_0(p_negFy_0), negFcapFy_0(p_negFcapFy_0), negFresFy_0(p_negFresFy_0),
    LAMBDA_S(p_LAMBDA_S), LAMBDA_C(p_LAMBDA_C), LAMBDA_A(p_LAMBDA_A), LAMBDA_K(p_LAMBDA_K), c_S(p_c_S), c_C(p_c_C), c_A(p_c_A), c_K(p_c_K), D_pos(p_D_pos), D_neg(p_D_neg), kappaF(p_kappaF), kappaD(p_kappaD)
{
	this->revertToStart();
}

IMKPinching::IMKPinching()
    :UniaxialMaterial(0, 0), Ke(0),
    posUp_0(0), posUpc_0(0), posUu_0(0), posFy_0(0), posFcapFy_0(0), posFresFy_0(0),
    negUp_0(0), negUpc_0(0), negUu_0(0), negFy_0(0), negFcapFy_0(0), negFresFy_0(0),
    LAMBDA_S(0), LAMBDA_C(0), LAMBDA_A(0), LAMBDA_K(0), c_S(0), c_C(0), c_A(0), c_K(0), D_pos(0), D_neg(0), kappaF(0), kappaD(0)
{
	this->revertToStart();
}

IMKPinching::~IMKPinching()
{
	// does nothing
}

int IMKPinching::setTrialStrain(double strain, double strainRate)
{
	//all variables to the last commit
	this->revertToLastCommit();

    //state determination algorithm: defines the current force and tangent stiffness
    double Ui_1 = Ui;
    double Fi_1 = Fi;
    U           = strain; //set trial displacement
    Ui          = U;
    double dU   = Ui - Ui_1;    // Incremental deformation at current step
    double dEi  = 0;
    KgetTangent = Ktangent;
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////  MAIN CODE //////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    if (Failure_Flag) {     // When a failure has already occured
        Fi  = 0;
    } else if (dU == 0) {   // When deformation doesn't change from the last
        Fi  = Fi_1;
    } else {
    ///////////////////////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////// FLAG RAISE ////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////////////////
    //      Excursion_Flag: When crossing X-axis.  Evokes re-considering of the deteriorations and which peak to go for.
    //      Reversal_Flag:  When unloading starts. Evokes re-condiersing of the stiffness deterioration and peak point registration.
        double  betaS=0,betaC=0,betaK=0,betaA=0;
        bool    FailS=false,FailC=false,FailK=false,FailA=false;
        double  dF;
        double  Kglobal,Klocal,Kpinch;
        int     exBranch        = Branch;
        bool    Excursion_Flag  = false;
        bool    Reversal_Flag   = false;
        if (Branch == 0) {                          // Elastic Branch
            if (Ui > posUy) {                           // Yield in Positive
                Branch 	= 5;
            } else if (Ui < negUy) {                    // Yield in Negative
                Branch 	= 15;
            }
        } else if (Branch == 1) {                   // Unloading Branch
            if (Fi_1*(Fi_1+dU*Kunload) <= 0) {          // Crossing X-axis and Reloading Towards Opposite Direction
                Excursion_Flag 	= true;
            } else if (Fi_1 > 0 && Ui > posUlocal) {    // Back to Reloading (Positive)
                Kpinch  = (Fpinch       - posFlocal) / (Upinch      - posUlocal);
                Kglobal = (posFglobal   - posFlocal) / (posUglobal  - posUlocal);
                if (posUlocal < Upinch && posFlocal < Fpinch && Upinch < posUglobal && Fpinch < posFglobal && Kpinch < Kglobal) {
                    Branch  = 2;                        // Pinching Branch
                } else {
                    Branch  = 4;                        // Towards Global Peak
                }
            } else if (Fi_1 < 0 && Ui < negUlocal) {    // Back to Reloading (Negative)
                Kpinch  = (Fpinch       - negFlocal) / (Upinch      - negUlocal);
                Kglobal = (negFglobal   - negFlocal) / (negUglobal  - negUlocal);
                if (negUglobal < Upinch && negFglobal < Fpinch && Upinch < negUlocal && Fpinch < negFlocal && Kpinch < Kglobal) {
                    Branch  = 12;                       // Pinching Branch
                } else {
                    Branch  = 14;                       // Towards Global Peak
                }
            }
        } else if (Fi_1*dU < 0) {                   // Reversal from Reloading or Backbone Section
            Reversal_Flag  	= true;
            Branch 	= 1;
        }
    ///////////////////////////////////////////////////////////////////////////////////////////
    /////////////////// WHEN REVERSAL /////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////////////////
        if (Reversal_Flag) {
    /////////////////////////// UPDATE PEAK POINTS ////////////////////////////////////////////
            if ( Fi_1 > 0 ){
                posUlocal	= Ui_1;           // UPDATE LOCAL
                posFlocal	= Fi_1;
                if ( Ui_1 > posUglobal ) {    // UPDATE GLOBAL
                    posUglobal   	= Ui_1;
                    posFglobal   	= Fi_1;
                }
            } else {
                negUlocal	= Ui_1;           // UPDATE LOCAL
                negFlocal	= Fi_1;
                if ( Ui_1 < negUglobal ) {    // UPDATE GLOBAL
                    negUglobal   	= Ui_1;
                    negFglobal   	= Fi_1;
                }
            }
    /////////////////// UPDATE UNLOADING STIFFNESS ////////////////////////////////////////////
            double  EpjK    = engAcml            - 0.5*(Fi_1 / Kunload)*Fi_1;
            double  EiK     = engAcml - engDspt  - 0.5*(Fi_1 / Kunload)*Fi_1;
            betaK           = pow( (EiK / (engRefK - EpjK)), c_K );
            FailK           = (betaK>1);
            betaK           = betaK < 0 ? 0 : (betaK>1 ? 1 : betaK);
            Kunload         *= (1 - betaK);
            Ktangent        = Kunload;
        // Detect unloading completed in a step.
            if (Fi_1*(Fi_1+dU*Kunload) <= 0) {
                exBranch        = 1;
                Excursion_Flag  = true;
                Reversal_Flag   = false;
            }
        }
    ///////////////////////////////////////////////////////////////////////////////////////////
    /////////////////// WHEN NEW EXCURSION /////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////////////////
        if ( Excursion_Flag ) {
    /////////////////// UPDATE BACKBONE CURVE /////////////////////////////////////////////////
            double FcapProj, FyProj;
            double Ei   = max(0.0, engAcml - engDspt);
            betaS       = pow((Ei / (engRefS - engAcml)), c_S);
            betaC       = pow((Ei / (engRefC - engAcml)), c_C);
            betaA       = pow((Ei / (engRefA - engAcml)), c_A);
            FailS       = (betaS>1);
            FailC       = (betaC>1);
            FailA       = (betaA>1);
            betaS       = betaS < 0 ? 0 : (betaS>1 ? 1 : betaS);
            betaC       = betaC < 0 ? 0 : (betaC>1 ? 1 : betaC);
            betaA       = betaA < 0 ? 0 : (betaA>1 ? 1 : betaA);
            engDspt     = engAcml;
        // Positive
            if (dU > 0) {
                FcapProj    = posFcap  - posKpc * posUcap;
            // Yield Point
                posFy       *= (1 - betaS * D_pos);
                posKp       *= (1 - betaS * D_pos); // Post-Yield Stiffness
                FcapProj    *= (1 - betaC * D_pos);
                posUglobal  *= (1 + betaA * D_pos); // Accelerated Reloading Stiffness
                if (posFy < posFres) {
                    posFy   = posFres;
                    posFcap = posFres;
                    posKp   = 0;
                    posKpc  = 0;
                    posUy   = posFy / Ke;
                    posUcap = 0;
                } else {
                    posUy   = posFy / Ke;
                // Capping Point
                    FyProj      = posFy - posKp*posUy;
                    posUcap     = (FcapProj - FyProj) / (posKp - posKpc);
                    posFcap     = FyProj + posKp*posUcap;
                }
            // Global Peak on the Updated Backbone
                if (posUglobal < posUy) {           // Elastic Branch
                    posFglobal = Ke * posUglobal;
                }
                else if (posUglobal < posUcap) {    // Post-Yield Branch
                    posFglobal = posFy   + posKp  * (posUglobal - posUy);
                }
                else {                              // Post-Capping Branch
                    posFglobal = posFcap + posKpc * (posUglobal - posUcap);
                }
                if (posFglobal < posFres) {     // Residual Branch
                    posFglobal = posFres;
                }
                posUres = (posFres - posFcap + posKpc * posUcap) / posKpc;
            }

        // Negative
            else {
                FcapProj    = negFcap   - negKpc * negUcap;
            // Yield Point
                negFy	    *= (1 - betaS * D_neg);
                negKp	    *= (1 - betaS * D_neg); // Post-Yield Stiffness
                FcapProj    *= (1 - betaC * D_neg);
                negUglobal	*= (1 + betaA * D_neg); // Accelerated Reloading Stiffness
                if (negFy > negFres) {
                    negFy	= negFres;
                    negFcap = negFres;
                    negKp	= 0;
                    negKpc  = 0;
                    negUy   = negFy/Ke;
                    negUcap = 0;
                } else {
                    negUy   = negFy / Ke;
                // Capping Point
                    FyProj      = negFy - negKp*negUy;
                    negUcap     = (FcapProj - FyProj) / (negKp - negKpc);
                    negFcap     = FyProj + negKp*negUcap;
                }
            // Global Peak on the Updated Backbone
                if (negUy < negUglobal) {           // Elastic Branch
                    negFglobal	= Ke * negUglobal; 
                }
                else if (negUcap < negUglobal) {    // Post-Yield Branch
                    negFglobal	= negFy   + negKp  * (negUglobal - negUy);
                }
                else {                              // Post-Capping Branch
                    negFglobal  = negFcap + negKpc * (negUglobal - negUcap);
                }
                if (negFres < negFglobal) {     // Residual Branch
                    negFglobal  = negFres;
                }
                negUres  	= (negFres - negFcap + negKpc * negUcap) / negKpc;
            }

    ///////////////////////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////// RELOADING TARGET DETERMINATION /////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////////////////
            double  u0 	= Ui_1 - (Fi_1 / Kunload);
            double  Uplstc;
            if (dU > 0) {
                Uplstc      = posUglobal    - (posFglobal / Kunload);
                Upinch      = (1-kappaD)*Uplstc;
                Fpinch      = kappaF*posFglobal*(Upinch-u0)/(posUglobal-u0);
                Kpinch      = Fpinch        / (Upinch       - u0);
                Kglobal   	= posFglobal    / (posUglobal - u0);
                Klocal    	= posFlocal     / (posUlocal  - u0);
                if (u0 < Upinch) {
                    Branch     	= 2;
                    Ktangent   	= Kpinch;
                }
                else  if ( u0 < posUlocal && posFlocal < posFglobal && Klocal > Kglobal) {
                    Branch      = 3;
                    Ktangent     = Klocal;
                }
                else {
                    Branch      = 4;
                    Ktangent     = Kglobal;
                }
            }
            else {
                Uplstc      = negUglobal    - (negFglobal / Kunload);
                Upinch      = (1-kappaD)*Uplstc;
                Fpinch      = kappaF*negFglobal*(Upinch-u0)/(negUglobal-u0);
                Kpinch      = Fpinch        / (Upinch       - u0);
                Kglobal     = negFglobal    / (negUglobal - u0);
                Klocal    	= negFlocal     / (negUlocal  - u0);
                if (u0 > Upinch) {
                    Branch     	= 12;
                    Ktangent   	= Kpinch;
                }
                else  if ( u0 > negUlocal && negFlocal > negFglobal && Klocal > Kglobal) {
                    Branch      = 13;
                    Ktangent     = Klocal;
                }
                else {
                    Branch      = 14;
                    Ktangent     = Kglobal;
                }
            }
         }
    ///////////////////////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////////////////
    ////////////////// BRANCH SHIFT CHECK /////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////////////////
    //  Branch
    //      0:  Elastic
    //      1:  Unloading Branch
    //      2:  Towards Pinching Point  +
    //      3:  Towards Local Peak      +
    //      4:  Towards Global Peak     +
    //      5:  Towards Capping Point   +
    //      6:  Towards Residual Point  +
    //      7:  Residual Branch         +
    //      12: Towards Pinching Point  -
    //      13: Towards Local Peak      -
    //      14: Towards Global Peak     -
    //      15: Towards Capping Point   -
    //      16: Towards Residual Point  -
    //      17: Residual Branch         -
    // Branch shifting from 2 -> 3 -> 4 -> 5 -> 6 -> 7
        if (Branch == 2 && Ui > Upinch) {
            exBranch    = 2;
            Kglobal     = (posFglobal   - Fpinch) / (posUglobal - Upinch);
            Klocal      = (posFlocal    - Fpinch) / (posUlocal  - Upinch);
            if (Upinch < posUlocal && Fpinch < posFlocal && posFlocal < posFglobal && Klocal > Kglobal) {
                Branch  = 3;
            } else {
                Branch 	= 4;
            }

        }
        if (Branch == 3 && Ui > posUlocal) {
            exBranch    = 3;
            Branch      = 4;
        }
        if (Branch == 4 && Ui > posUglobal) {
            exBranch    = 4;
            Branch 	    = 5;
        }
        if (Branch == 5 && Ui > posUcap) {
            exBranch    = 5;
            Branch 	    = 6;
        }
        if (Branch == 6 && Ui > posUres) {
            exBranch    = 6;
            Branch 	    = 7;
        }

        if (Branch == 12 && Ui < Upinch) {
            exBranch    = 12;
            Kglobal     = (negFglobal   - Fpinch) / (negUglobal - Upinch);
            Klocal      = (negFlocal    - Fpinch) / (negUlocal  - Upinch);
            if (negFglobal < negFlocal && negUlocal < Upinch && negFlocal < Fpinch && Klocal > Kglobal) {
                Branch  = 13;
            } else {
                Branch 	= 14;
            }
        }
        if (Branch == 13 && Ui < negUlocal) {
            exBranch    = 13;
            Branch      = 14;
        }
        if (Branch == 14 && Ui < negUglobal) {
            exBranch    = 14;
            Branch 	    = 15;
        }
        if (Branch == 15 && Ui < negUcap) {
            exBranch    = 15;
            Branch 	    = 16;
        }
        if (Branch == 16 && Ui < negUres) {
            exBranch    = 16;
            Branch 	    = 17;
        }

    ///////////////////////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////////////////
    ///////////////////////////////// COMPUTE FORCE INCREMENT /////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////////////////
    // Without Branch Change
        if (Branch == exBranch || Branch == 1) {
            dF	= dU*Ktangent;
        }
    // With Branch Change
    // Positive Force
        else if (exBranch == 1 && Excursion_Flag) {
            double  u0 	= Ui_1 - (Fi_1 / Kunload);
            dF 	= 0             - Fi_1 + Ktangent*  (Ui - u0);
        }
        else if (Branch == 2) {
            Ktangent    = (Fpinch   - posFlocal)    / (Upinch       - posUlocal);
            dF          = posFlocal - Fi_1          + Ktangent*(Ui   - posUlocal);
        }
        else if (Branch == 3) {
            Ktangent    = (posFlocal - Fpinch)    / (posUlocal - Upinch);
            dF          = Fpinch - Fi_1          + Ktangent*(Ui   - Upinch);
        }
        else if (Branch == 4 && exBranch == 2) {
            Ktangent   	= (posFglobal   - Fpinch)   / (posUglobal   - Upinch);
            dF 	        = Fpinch        - Fi_1      + Ktangent*(Ui   - Upinch);
        }
        // CASE 4: WHEN RELOADING BUT BETWEEN LAST CYCLE PEAK POINT AND GLOBAL PEAK POINT
        // CASE 5: WHEN LOADING IN GENERAL TOWARDS THE TARGET PEAK
        // CASE 6: WHEN LOADING IN GENERAL TOWARDS THE LAST CYCLE PEAK POINT BUT BEYOND IT
        else if (Branch == 4) {
            Ktangent   	= (posFglobal   - posFlocal)    / (posUglobal - posUlocal);
            dF      	= posFlocal     - Fi_1          + Ktangent*(Ui - posUlocal);
        }
        // CASE 7: WHEN LOADING BEYOND THE TARGET PEAK BUT BEFORE THE CAPPING POINT
        else if (Branch == 5 && exBranch == 0) {
            dF 	= posFy       - Fi_1 + posKp*   (Ui - posUy);
            Ktangent	= posKp;
        }
        else if (Branch == 5) {
            dF 	= posFglobal  - Fi_1 + posKp*   (Ui - posUglobal);
            Ktangent	= posKp;
        }
        // CASE 8: WHEN LOADING AND BETWEEN THE CAPPING POINT AND THE RESIDUAL POINT
        else if (Branch == 6 && exBranch == 5) {
            dF 	= posFcap     - Fi_1 + posKpc*  (Ui - posUcap);
            Ktangent	= posKpc;
        }
        else if (Branch == 6) {
            dF 	= posFglobal  - Fi_1 + posKpc*  (Ui - posUglobal);
            Ktangent	= posKpc;
        }
        // CASE 9: WHEN LOADING AND BEYOND THE RESIDUAL POINT
        else if (Branch == 7) {
            dF 	= posFres     - Fi_1;
            Ktangent	= 0;
    // Negative Force
        }
        else if (Branch == 12) {
            Ktangent     = (Fpinch   - negFlocal)    / (Upinch       - negUlocal);
            dF          = negFlocal - Fi_1          + Ktangent*(Ui   - negUlocal);
        }
        else if (Branch == 13) {
            Ktangent     = (negFlocal - Fpinch)    / (negUlocal - Upinch);
            dF          = Fpinch - Fi_1          + Ktangent*(Ui   - Upinch);
        }
        else if (Branch == 14 && exBranch == 12) {
            Ktangent   	= (negFglobal   - Fpinch)   / (negUglobal   - Upinch);
            dF 	        = Fpinch        - Fi_1      + Ktangent*(Ui   - Upinch);
        }
        else if (Branch == 14) {
        // CASE 4: WHEN RELOADING BUT BETWEEN LAST CYCLE PEAK POINT AND GLOBAL PEAK POINT
        // CASE 5: WHEN LOADING IN GENERAL TOWARDS THE TARGET PEAK
        // CASE 6: WHEN LOADING IN GENERAL TOWARDS THE LAST CYCLE PEAK POINT BUT BEYOND IT
            Ktangent   	= (negFglobal - negFlocal) / (negUglobal - negUlocal);
            dF         	= negFlocal - Fi_1 + Ktangent*(Ui - negUlocal);
        }
        // CASE 7: WHEN LOADING BEYOND THE TARGET PEAK BUT BEFORE THE CAPPING POINT
        else if (Branch == 15 && exBranch == 0) {
            dF 	= negFy - Fi_1 + negKp*(Ui - negUy);
            Ktangent	= negKp;
        }
        else if (Branch == 15) {
            dF 	= negFglobal - Fi_1 + negKp*(Ui - negUglobal);
            Ktangent	= negKp;
        }
        // CASE 8: WHEN LOADING AND BETWEEN THE CAPPING POINT AND THE RESIDUAL POINT
        else if (Branch == 16 && exBranch == 15) {
            dF 	= negFcap - Fi_1 + negKpc*(Ui - negUcap);
            Ktangent	= negKpc;
        }
        else if (Branch == 16) {
            dF 	= negFglobal - Fi_1 + negKpc*(Ui - negUglobal);
            Ktangent	= negKpc;
        }
        // CASE 9: WHEN LOADING AND BEYOND THE RESIDUAL POINT
        else if (Branch == 17) {
            dF 	= negFres - Fi_1;
            Ktangent	= 0;
        }
    // Branch Change check
        // if (Branch!=exBranch) {
        //  std::cout << exBranch << " -> " << Branch << "\n";
        // }
// Force
        Fi	= Fi_1 + dF;
    ///////////////////////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////////////////
    // CHECK FOR FAILURE
    ///////////////////////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////////////////
        // Failure criteria (Tolerance	= 1//)
    // I have no idea about why it can' t be 0 nor 1.
        bool	FailPp 	= ( posFglobal == 0               );
        bool	FailPn 	= ( negFglobal == 0               );
        bool	FailDp 	= ( Ui >  posUu_0                 );
        bool	FailDn 	= ( Ui < -negUu_0                 );
        bool	FailRp 	= ( Branch ==  7 && posFres == 0  );
        bool	FailRn 	= ( Branch == 17 && negFres == 0  );
        if (FailS||FailC||FailA||FailK||FailPp||FailPn||FailRp||FailRn||FailDp||FailDn) {
            Fi  = 0;
            Failure_Flag    = true;
        }

        dEi	= 0.5*(Fi + Fi_1)*dU;   // Internal energy increment

        if (KgetTangent!=Ktangent) {
            KgetTangent  = (Fi - Fi_1) / dU;
        }
    }
    engAcml	+= dEi; 	            // Energy
    if (KgetTangent==0) {
        KgetTangent  = 1e-6;
    }
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////// END OF MAIN CODE ///////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    return 0;
}

double IMKPinching::getStress(void)
{
	//cout << " getStress" << endln;
	return (Fi);
}

double IMKPinching::getTangent(void)
{
    //cout << " getTangent" << endln;
    return (KgetTangent);
}

double IMKPinching::getInitialTangent(void)
{
	//cout << " getInitialTangent" << endln;
	return (Ke);
}

double IMKPinching::getStrain(void)
{
	//cout << " getStrain" << endln;
	return (U);
}

int IMKPinching::commitState(void)
{
	//cout << " commitState" << endln;

    //commit trial  variables
// 12 Pos U and F
    cPosUy	    = posUy;
    cPosFy	    = posFy;
    cPosUcap	= posUcap;
    cPosFcap	= posFcap;
    cPosUlocal	= posUlocal;
    cPosFlocal	= posFlocal;
    cPosUglobal	= posUglobal;
    cPosFglobal	= posFglobal;
    cPosUres	= posUres;
    cPosFres	= posFres;
    cPosKp	    = posKp;
    cPosKpc	    = posKpc;
// 12 Neg U and F
    cNegUy	    = negUy;
    cNegFy	    = negFy;
    cNegUcap	= negUcap;
    cNegFcap	= negFcap;
    cNegUlocal	= negUlocal;
    cNegFlocal	= negFlocal;
    cNegUglobal	= negUglobal;
    cNegFglobal	= negFglobal;
    cNegUres	= negUres;
    cNegFres	= negFres;
    cNegKp	    = negKp;
    cNegKpc	    = negKpc;
// 2 Pinching
    cFpinch     = Fpinch;
    cUpinch     = Upinch;
// 3 State
    cU		    = U;
    cUi	    	= Ui;
    cFi	        = Fi;
// 2 Stiffness
    cKtangent	= Ktangent;
    cKunload	= Kunload;
// 2 Energy
    cEngAcml	= engAcml;
    cEngDspt	= engDspt;
// 2 Flag
    cFailure_Flag   = Failure_Flag;
    cBranch         = Branch;
    return 0;
}

int IMKPinching::revertToLastCommit(void)
{
    //cout << " revertToLastCommit" << endln;
    //the opposite of commit trial history variables
// 12 Positive U and F
    posUy	        = cPosUy;
    posFy	        = cPosFy;
    posUcap	        = cPosUcap;
    posFcap	        = cPosFcap;
    posUlocal	    = cPosUlocal;
    posFlocal	    = cPosFlocal;
    posUglobal	    = cPosUglobal;
    posFglobal	    = cPosFglobal;
    posUres	        = cPosUres;
    posFres	        = cPosFres;
    posKp	        = cPosKp;
    posKpc	        = cPosKpc;
// 12 Negative U and F
    negUy	        = cNegUy;
    negFy	        = cNegFy;
    negUcap	        = cNegUcap;
    negFcap	        = cNegFcap;
    negUlocal	    = cNegUlocal;
    negFlocal	    = cNegFlocal;
    negUglobal	    = cNegUglobal;
    negFglobal	    = cNegFglobal;
    negUres	        = cNegUres;
    negFres	        = cNegFres;
    negKp       	= cNegKp;
    negKpc	        = cNegKpc;
// 2 Pinching
    Fpinch          = cFpinch;
    Upinch          = cUpinch;
// 3 State Variables
    U	            = cU;
    Ui	            = cUi;
    Fi	            = cFi;
// 2 Stiffness
    Ktangent	    = cKtangent;
    Kunload	        = cKunload;
// 2 Energy
    engAcml	        = cEngAcml;
    engDspt	        = cEngDspt;
// 2 Flag
    Failure_Flag	= cFailure_Flag;
    Branch          = cBranch;
    return 0;
}

int IMKPinching::revertToStart(void)
{
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////\\
    //////////////////////////////////////////////////////////////////// ONE TIME CALCULATIONS ////////////////////////////////////////////////////////////////////\\
    //////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////*/
// 14 Initial Values
    posUy_0  	= posFy_0   / Ke;
    posUcap_0	= posUy_0   + posUp_0;
    posFcap_0	= posFcapFy_0*posFy_0;
    posKp_0 	= (posFcap_0 - posFy_0) / posUp_0;
    posKpc_0 	= posFcap_0 / posUpc_0;
    negUy_0 	= negFy_0   / Ke;
    negUcap_0	= negUy_0   + negUp_0;
    negFcap_0	= negFcapFy_0*negFy_0;
    negKp_0 	= (negFcap_0 - negFy_0) / negUp_0;
    negKpc_0 	= negFcap_0 / negUpc_0;
    engRefS	    = LAMBDA_S  * posFy_0;
    engRefC	    = LAMBDA_C  * posFy_0;
    engRefA	    = LAMBDA_A  * posFy_0;
    engRefK	    = LAMBDA_K  * posFy_0;
// 12 Positive U and F
    posUy		= cPosUy	    = posUy_0;
    posFy    	= cPosFy 	    = posFy_0;
    posUcap  	= cPosUcap	    = posUcap_0;
    posFcap  	= cPosFcap	    = posFcap_0;
    posUlocal	= cPosUlocal	= posUy_0;
    posFlocal	= cPosFlocal	= posFy_0;
    posUglobal	= cPosUglobal	= posUy_0;
    posFglobal	= cPosFglobal	= posFy_0;
    posFres  	= cPosFres	    = posFy_0*posFresFy_0;
    posKp    	= cPosKp 	    =  posKp_0;
    posKpc   	= cPosKpc   	= -posKpc_0;
    posUres	    = cPosUres	    = (posFres - posFcap) / posKpc + posUcap;
// 12 Negative U and F
    negUy    	= cNegUy	    = -negUy_0;
    negFy    	= cNegFy	    = -negFy_0;
    negUcap  	= cNegUcap	    = -negUcap_0;
    negFcap  	= cNegFcap	    = -negFcap_0;
    negUlocal	= cNegUlocal	= -negUy_0;
    negFlocal	= cNegFlocal	= -negFy_0;
    negUglobal	= cNegUglobal	= -negUy_0;
    negFglobal	= cNegFglobal	= -negFy_0;
    negFres  	= cNegFres	    = -negFy_0*negFresFy_0;
    negKp    	= cNegKp	    =  negKp_0;
    negKpc   	= cNegKpc	    = -negKpc_0;
    negUres	    = cNegUres	    = (negFres - negFcap) / negKpc + negUcap;
// 3 State Values
    U	        = cU	        = 0;
    Ui      	= cUi 	        = 0;
    Fi 	        = cFi 	        = 0;
// 2 Stiffness
    Ktangent	= cKtangent	    = Ke;
    Kunload	    = cKunload	    = Ke;
// 2 Energy
    engAcml 	= cEngAcml	    = 0.0;
    engDspt	    = cEngDspt	    = 0.0;
// 2 Flag
    Failure_Flag    = cFailure_Flag = false;
    Branch      	= cBranch       = 0;
// 2 Pinching
    Fpinch      = cFpinch       = 0.0;
    Upinch      = cUpinch       = 0.0;
    //cout << " revertToStart:" << endln; //<< " U=" << U << " Ui=" << Ui << " TanK=" << Ktangent << endln;
    return 0;
}

UniaxialMaterial *
IMKPinching::getCopy(void)
{
    IMKPinching *theCopy = new IMKPinching(this->getTag(), Ke,
        posUy_0, posUcap_0, posUu_0, posFy_0, posFcapFy_0, posFresFy_0,
        negUy_0, negUcap_0, negUu_0, negFy_0, negFcapFy_0, negFresFy_0,
        LAMBDA_S, LAMBDA_C, LAMBDA_A, LAMBDA_K, c_S, c_C, c_A, c_K, D_pos, D_neg, kappaF, kappaD);

    //cout << " getCopy" << endln;
// 12 Positive U and F
    theCopy->posUy      = posUy;
    theCopy->posFy      = posFy;
    theCopy->posUcap    = posUcap;
    theCopy->posFcap    = posFcap;
    theCopy->posUlocal  = posUlocal;
    theCopy->posFlocal  = posFlocal;
    theCopy->posUglobal = posUglobal;
    theCopy->posFglobal = posFglobal;
    theCopy->posUres    = posUres;
    theCopy->posFres    = posFres;
    theCopy->posKp      = posKp;
    theCopy->posKpc     = posKpc;
// 12 Negative U and F
    theCopy->negUy      = negUy;
    theCopy->negFy      = negFy;
    theCopy->negUcap    = negUcap;
    theCopy->negFcap    = negFcap;
    theCopy->negUlocal  = negUlocal;
    theCopy->negFlocal  = negFlocal;
    theCopy->negUglobal = negUglobal;
    theCopy->negFglobal = negFglobal;
    theCopy->negUres    = negUres;
    theCopy->negFres    = negFres;
    theCopy->negKp      = negKp;
    theCopy->negKpc     = negKpc;
// 3 State Values
    theCopy->U          = U;
    theCopy->Ui         = Ui;
    theCopy->Fi         = Fi;
// 2 Stiffness
    theCopy->Ktangent   = Ktangent;
    theCopy->Kunload    = Kunload;
// 2 Energy
    theCopy->engAcml    = engAcml;
    theCopy->engDspt    = engDspt;
// 2 Flag
    theCopy->Failure_Flag   = Failure_Flag;
    theCopy->Branch     = Branch;
// 12 Positive U and F
    theCopy->cPosUy     = cPosUy;
    theCopy->cPosFy     = cPosFy;
    theCopy->cPosUcap   = cPosUcap;
    theCopy->cPosFcap   = cPosFcap;
    theCopy->cPosUlocal = cPosUlocal;
    theCopy->cPosFlocal = cPosFlocal;
    theCopy->cPosUglobal= cPosUglobal;
    theCopy->cPosFglobal= cPosFglobal;
    theCopy->cPosUres   = cPosUres;
    theCopy->cPosFres   = cPosFres;
    theCopy->cPosKp     = cPosKp;
    theCopy->cPosKpc    = cPosKpc;
// 12 Negative U and F
    theCopy->cNegUy     = cNegUy;
    theCopy->cNegFy     = cNegFy;
    theCopy->cNegUcap   = cNegUcap;
    theCopy->cNegFcap   = cNegFcap;
    theCopy->cNegUglobal= cNegUglobal;
    theCopy->cNegFglobal= cNegFglobal;
    theCopy->cNegUlocal = cNegUlocal;
    theCopy->cNegFlocal = cNegFlocal;
    theCopy->cNegUres   = cNegUres;
    theCopy->cNegFres   = cNegFres;
    theCopy->cNegKp     = cNegKp;
    theCopy->cNegKpc    = cNegKpc;
// 3 State
    theCopy->cU         = cU;
    theCopy->cUi        = cUi;
    theCopy->cFi        = cFi;
// 2 Stiffness
    theCopy->cKtangent  = cKtangent;
    theCopy->cKunload   = cKunload;
// 2 Energy
    theCopy->cEngAcml   = cEngAcml;
    theCopy->cEngDspt   = cEngDspt;
// 2 Pinching
    theCopy->cFpinch    = Fpinch;
    theCopy->cUpinch    = Upinch;
// 2 Flag
    theCopy->cFailure_Flag  = cFailure_Flag;
    theCopy->cBranch    = Branch;
    return theCopy;
}

int IMKPinching::sendSelf(int cTag, Channel &theChannel)
{
	int res = 0;
	cout << " sendSelf" << endln;

    static Vector data(137);
    data(0) = this->getTag();
// 25 Fixed Input Material Parameters 1-25
    data(1)  	= Ke;
    data(2)  	= posUp_0;
    data(3)  	= posUpc_0;
    data(4)  	= posUu_0;
    data(5)  	= posFy_0;
    data(6)  	= posFcapFy_0;
    data(7)  	= posFresFy_0;
    data(8)  	= negUp_0;
    data(9)  	= negUpc_0;
    data(10) 	= negUu_0;
    data(11) 	= negFy_0;
    data(12) 	= negFcapFy_0;
    data(13) 	= negFresFy_0;
    data(14) 	= LAMBDA_S;
    data(15) 	= LAMBDA_C;
    data(16) 	= LAMBDA_A;
    data(17) 	= LAMBDA_K;
    data(18) 	= c_S;
    data(19) 	= c_C;
    data(20) 	= c_A;
    data(21) 	= c_K;
    data(22) 	= D_pos;
    data(23) 	= D_neg;
    data(24)    = kappaF;
    data(25)    = kappaD;
// 14 Initial Values 31-44
    data(31)	= posUy_0;
    data(32)	= posUcap_0;
    data(33)	= posFcap_0;
    data(34)	= posKp_0;
    data(35)	= posKpc_0;
    data(36)	= negUy_0;
    data(37)	= negUcap_0;
    data(38)	= negFcap_0;
    data(39)	= negKp_0;
    data(40)	= negKpc_0;
    data(41)	= engRefS;
    data(42)	= engRefC;
    data(43)	= engRefA;
    data(44)	= engRefK;
// 12 Positive U and F 51-62
    data(51) 	= posUy;
    data(52) 	= posFy;
    data(53) 	= posUcap;
    data(54) 	= posFcap;
    data(55) 	= posUlocal;
    data(56) 	= posFlocal;
    data(57) 	= posUglobal;
    data(58) 	= posFglobal;
    data(59) 	= posUres;
    data(60) 	= posFres;
    data(51) 	= posKp;
    data(62) 	= posKpc;
// 3 State Variables 63-65
    data(63)    = U;
    data(64) 	= Ui;
    data(65) 	= Fi;
// 2 Stiffness 66 67
    data(66)	= Ktangent;
    data(67) 	= Kunload;
// 2 Energy 68 69
    data(68) 	= engAcml;
    data(69) 	= engDspt;
// 12 Negative U and F 71-82
    data(71) 	= negUy;
    data(72) 	= negFy;
    data(73) 	= negUcap;
    data(74) 	= negFcap;
    data(75) 	= negUlocal;
    data(76) 	= negFlocal;
    data(77) 	= negUglobal;
    data(78) 	= negFglobal;
    data(79) 	= negUres;
    data(80) 	= negFres;
    data(81) 	= negKp;
    data(82) 	= negKpc;
// 2 Pinching 83 84
    data(83)    = Fpinch;
    data(84)    = Upinch;
// 2 Flag 85 86
    data(85)	= Failure_Flag;
    data(86) 	= Branch;
// 12 Positive U and F 101-112
    data(101)	= cPosUy;
    data(102)	= cPosFy;
    data(103)	= cPosUcap;
    data(104)	= cPosFcap;
    data(105)	= cPosUlocal;
    data(106)	= cPosFlocal;
    data(107)	= cPosUglobal;
    data(108)	= cPosFglobal;
    data(109)	= cPosUres;
    data(110)	= cPosFres;
    data(111)	= cPosKp;
    data(112)	= cPosKpc;
// 3 State Variables 113-115
    data(113)   = cU;
    data(114)	= cUi;
    data(115)	= cFi;
// 2 Stiffness 116 117
    data(116)   = cKtangent;
    data(117)	= cKunload;
// 2 Energy 118 119
    data(118)   = cEngAcml;
    data(119)   = cEngDspt;
// 12 Negative U and F 121-132
    data(121)	= cNegUy;
    data(122)	= cNegFy;
    data(123)	= cNegUcap;
    data(124)	= cNegFcap;
    data(125)	= cNegUlocal;
    data(126)	= cNegFlocal;
    data(127)	= cNegUglobal;
    data(128)	= cNegFglobal;
    data(129)	= cNegUres;
    data(130)	= cNegFres;
    data(131)	= cNegKp;
    data(132)	= cNegKpc;
// 2 Pinching 133 134
    data(133)   = cFpinch;
    data(134)   = cUpinch;
// 2 Flag 135 136
    data(135)	= cFailure_Flag;
    data(136)	= cBranch;
    res = theChannel.sendVector(this->getDbTag(), cTag, data);
    if (res < 0)
        opserr << "IMKPinching::sendSelf() - failed to send data\n";
	return res;
}

int IMKPinching::recvSelf(int cTag, Channel &theChannel, FEM_ObjectBroker &theBroker)
{
    int res = 0;
    static Vector data(137);
    res = theChannel.recvVector(this->getDbTag(), cTag, data);

    if (res < 0) {
        opserr << "IMKPinching::recvSelf() - failed to receive data\n";
        this->setTag(0);
    }
    else {
        cout << " recvSelf" << endln;
        this->setTag((int)data(0));
    // 25 Fixed Input Material Parameters
        Ke				= data(1);
        posUp_0			= data(2);
        posUpc_0		= data(3);
        posUu_0			= data(4);
        posFy_0			= data(5);
        posFcapFy_0		= data(6);
        posFresFy_0		= data(7);
        negUp_0			= data(8);
        negUpc_0		= data(9);
        negUu_0			= data(10);
        negFy_0			= data(11);
        negFcapFy_0		= data(12);
        negFresFy_0		= data(13);
        LAMBDA_S		= data(14);
        LAMBDA_C		= data(15);
        LAMBDA_A		= data(16);
        LAMBDA_K		= data(17);
        c_S				= data(18);
        c_C				= data(19);
        c_A				= data(20);
        c_K				= data(21);
        D_pos			= data(22);
        D_neg			= data(23);
        kappaF          = data(24);
        kappaD          = data(25);
    // 14 Initial Values
        posUy_0			= data(31);
        posUcap_0		= data(32);
        posFcap_0		= data(33);
        posKp_0			= data(34);
        posKpc_0		= data(35);
        negUy_0			= data(36);
        negUcap_0		= data(37);
        negFcap_0		= data(38);
        negKp_0			= data(39);
        negKpc_0		= data(40);
        engRefS			= data(41);
        engRefC			= data(42);
        engRefA			= data(43);
        engRefK			= data(44);
    // 12 Positive U and F
        posUy			= data(51);
        posFy			= data(52);
        posUcap		    = data(53);
        posFcap		   	= data(54);
        posUlocal	    = data(55);
        posFlocal	    = data(56);
        posUglobal	    = data(57);
        posFglobal	    = data(58);
        posUres	    	= data(59);
        posFres		   	= data(60);
        posKp			= data(61);
        posKpc		    = data(62);
    // 3 State Variables
        U               = data(63);
        Ui				= data(64);
        Fi				= data(65);
    // 2 Stiffness
        Ktangent		= data(66);
        Kunload	    	= data(67);
    // 2 Energy
        engAcml			= data(68);
        engDspt		    = data(69);
    // 12 Negative U and F
        negUy			= data(71);
        negFy			= data(72);
        negUcap		    = data(73);
        negFcap		   	= data(74);
        negUlocal	    = data(75);
        negFlocal  	    = data(76);
        negUglobal	    = data(77);
        negFglobal  	= data(78);
        negUres		   	= data(79);
        negFres		    = data(80);
        negKp			= data(81);
        negKpc		    = data(82);
    // 2 Pinching
        Fpinch          = data(83);
        Upinch          = data(84);
    // 2 Flag
        Failure_Flag	= data(85);
        Branch			= data(86);
    // 12 Positive U and F
        cPosUy			= data(101);
        cPosFy			= data(102);
        cPosUcap		= data(103);
        cPosFcap		= data(104);
        cPosUlocal		= data(105);
        cPosFlocal		= data(106);
        cPosUglobal		= data(107);
        cPosFglobal		= data(108);
        cPosUres		= data(109);
        cPosFres		= data(110);
        cPosKp			= data(111);
        cPosKpc			= data(112);
    // 3 State Variables
        cU              = data(113);
        cUi				= data(114);
        cFi				= data(115);
    // 2 Stiffness
        cKtangent       = data(116);
        cKunload		= data(117);
    // 2 Energy
        cEngAcml        = data(118);
        cEngDspt        = data(119);
    // 12 Negative U and F
        cNegUy			= data(121);
        cNegFy			= data(122);
        cNegUcap		= data(123);
        cNegFcap		= data(124);
        cNegUlocal		= data(125);
        cNegFlocal		= data(126);
        cNegUglobal		= data(127);
        cNegFglobal		= data(128);
        cNegUres		= data(129);
        cNegFres		= data(130);
        cNegKp			= data(131);
        cNegKpc			= data(132);
    // 2 Pinching
        cFpinch         = data(133);
        cUpinch         = data(134);
    // 2 Flag
        cFailure_Flag	= data(135);
        cBranch			= data(136);
    }

	return res;
}

void IMKPinching::Print(OPS_Stream &s, int flag)
{
	cout << "IMKPinching tag: " << this->getTag() << endln;
}