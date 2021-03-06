/*
 *
 *                 #####    #####   ######  ######  ###   ###
 *               ##   ##  ##   ##  ##      ##      ## ### ##
 *              ##   ##  ##   ##  ####    ####    ##  #  ##
 *             ##   ##  ##   ##  ##      ##      ##     ##
 *            ##   ##  ##   ##  ##      ##      ##     ##
 *            #####    #####   ##      ######  ##     ##
 *
 *
 *             OOFEM : Object Oriented Finite Element Code
 *
 *               Copyright (C) 1993 - 2013   Borek Patzak
 *
 *
 *
 *       Czech Technical University, Faculty of Civil Engineering,
 *   Department of Structural Mechanics, 166 29 Prague, Czech Republic
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */


#include "enhancedassumestrainelementinterface.h"
#include "domain.h"
#include "structuralcrosssection.h"
#include "structuralms.h"

		  


namespace oofem {


// constructor
EnhancedAssumedStrainElementExtensionInterface :: EnhancedAssumedStrainElementExtensionInterface(Domain *d)  : Interface()
{
	
}

// Left lower block of Stiffness Matrix 
void
EnhancedAssumedStrainElementExtensionInterface :: computeStiffnessMatrixGamma(FloatMatrix &answer, MatResponseMode mode, GaussPoint *gp, TimeStep *tStep, StructuralElement* elem)
{
	
	StructuralCrossSection *cs = elem->giveStructuralCrossSection();    
	answer.resize(this->computeNumberOfEnhancedDofs(), elem->computeNumberOfDofs(EID_MomentumBalance) );
	
	FloatMatrix G, D, DB, B;

	int nlGeometry;
	NLStructuralElement* nlElem = static_cast< NLStructuralElement * > ( elem );
	if( nlElem ) {
		nlGeometry = nlElem->nlGeometry;
	} else {
		nlGeometry = 0;
	}



    if ( nlGeometry == 0 ) {
		elem->computeBmatrixAt(gp, B);
		this->computeEnhancedBmatrixAt(gp, G,elem);
		elem->computeConstitutiveMatrixAt(D, mode, gp, tStep);
	} else if ( nlGeometry == 1 ) {
		if ( elem->domain->giveEngngModel()->giveFormulation() == AL ) { // Material stiffness dC/de
			nlElem->computeBHmatrixAt(gp, B);
			this->computeEnhancedBHmatrixAt(gp, G,nlElem);
			cs->giveStiffnessMatrix_dCde(D, mode, gp, tStep);
		} else { // Material stiffness dP/dF
			nlElem->computeBHmatrixAt(gp, B);
			this->computeEnhancedBHmatrixAt(gp, G,nlElem);
			cs->giveStiffnessMatrix_dPdF(D, mode, gp, tStep);
		}
	}
	double dV = elem->computeVolumeAround(gp);
	DB.beProductOf(D, B);
	answer.beTProductOf(G, DB);
	answer.times(dV);		
} 


// Right lower block of Stiffness Matrix 
void
EnhancedAssumedStrainElementExtensionInterface :: computeStiffnessMatrixH(FloatMatrix &answer, MatResponseMode mode, GaussPoint *gp, TimeStep *tStep, StructuralElement* elem)
{
	
	StructuralCrossSection *cs = elem->giveStructuralCrossSection();
	answer.resize(this->computeNumberOfEnhancedDofs(), this->computeNumberOfEnhancedDofs() );

	FloatMatrix G, D, DG;
	
	int nlGeometry;
	NLStructuralElement* nlElem = static_cast< NLStructuralElement * > ( elem );
	if( nlElem ) {
		nlGeometry = nlElem->nlGeometry;
	} else {
		nlGeometry = 0;
	}


	if ( nlGeometry == 0 ) {
		this->computeEnhancedBmatrixAt(gp, G,elem);
		elem->computeConstitutiveMatrixAt(D, mode, gp, tStep);
	} else if ( nlGeometry == 1 ) {
		if ( elem->domain->giveEngngModel()->giveFormulation() == AL ) { // Material stiffness dC/de
			this->computeEnhancedBHmatrixAt(gp, G,nlElem);
			cs->giveStiffnessMatrix_dCde(D, mode, gp, tStep);
		} else { // Material stiffness dP/dF
			this->computeEnhancedBHmatrixAt(gp, G,nlElem);
			cs->giveStiffnessMatrix_dPdF(D, mode, gp, tStep);
		}
	}
	double dV = elem->computeVolumeAround(gp);
	DG.beProductOf(D, G);
	answer.beTProductOf(G,DG);
	answer.times(dV);
} 


// Stiffness correction due to the enhanced strains obtained after static condensation
void
EnhancedAssumedStrainElementExtensionInterface :: computeStiffnessCorrection(FloatMatrix &answer, MatResponseMode mode, TimeStep *tStep, StructuralElement* elem)
{
	FloatMatrix H, h, Gamma, gamma, invH, junk;
    answer.resize( this->computeNumberOfEnhancedDofs(), this->computeNumberOfEnhancedDofs() );
	Gamma.resize( this->computeNumberOfEnhancedDofs(), elem->computeNumberOfDofs(EID_MomentumBalance) );
	H.resize( this->computeNumberOfEnhancedDofs(), this->computeNumberOfEnhancedDofs() );
    
	if ( !elem->isActivated(tStep) ) {
        return;
    }

    if ( elem->numberOfIntegrationRules == 1 ) {        
        IntegrationRule *iRule = elem->integrationRulesArray [ elem->giveDefaultIntegrationRule() ];
        for ( int j = 0; j < iRule->giveNumberOfIntegrationPoints(); j++ ) {
            GaussPoint *gp = iRule->getIntegrationPoint(j);
			this->computeStiffnessMatrixH(h, mode, gp, tStep,elem);
			H.add(h);
			this->computeStiffnessMatrixGamma(gamma, mode, gp, tStep,elem);
			Gamma.add(gamma);            
        }        
    }
	invH.beInverseOf(H);
	junk.beTProductOf(Gamma,H);
	answer.beProductOf(junk,Gamma);	

	this->setTempInvStiffnessMatrixH(invH);
	this->setTempStiffnessMatrixGamma(Gamma);
}

// Internal forces correction due to the enhanced strains
void
EnhancedAssumedStrainElementExtensionInterface ::  giveInternalForcesCorrectionVector(FloatArray &answer, TimeStep *tStep, StructuralElement* elem)
{
	
	
	FloatArray vStress, h_int;
	FloatMatrix B,Gamma, invH;	
	
	answer.resize( this->computeNumberOfEnhancedDofs() );
	h_int.resize( this->computeNumberOfEnhancedDofs() );
		    
	if ( !elem->isActivated(tStep) ) {
        return;
    }

	int nlGeometry;
	NLStructuralElement* nlElem = static_cast< NLStructuralElement * > ( elem );
	if( nlElem ) {
		nlGeometry = nlElem->nlGeometry;
	} else {
		nlGeometry = 0;
	}

		
	  

	
    if (elem-> numberOfIntegrationRules == 1 ) {        
        IntegrationRule *iRule = elem->integrationRulesArray [ elem->giveDefaultIntegrationRule() ];
		this->giveTempInvStiffnessMatrixH(invH);
		this->giveTempStiffnessMatrixGamma(Gamma);

        for ( int j = 0; j < iRule->giveNumberOfIntegrationPoints(); j++ ) {
            GaussPoint *gp = iRule->getIntegrationPoint(j);
			Material *mat = gp->giveMaterial();
	
			     
			
			if ( nlGeometry == 0 ) {
				this->computeEnhancedBmatrixAt(gp, B,elem);
                vStress = static_cast< StructuralMaterialStatus * >( mat->giveStatus(gp) )->giveStressVector();
			} else if ( nlGeometry == 1 ) {  
				if ( elem->domain->giveEngngModel()->giveFormulation() == AL ) { // Cauchy stress
					vStress = static_cast< StructuralMaterialStatus * >( mat->giveStatus(gp) )->giveCVector();
                    this->computeEnhancedBmatrixAt(gp, B,elem);
				} else { // First Piola-Kirchhoff stress
					vStress = static_cast< StructuralMaterialStatus * >( mat->giveStatus(gp) )->givePVector();
					this->computeEnhancedBHmatrixAt(gp, B,nlElem);
				}
			}
			if ( vStress.giveSize() == 0 ) {
				break;
			}
			double dV  = elem->computeVolumeAround(gp);
			h_int.plusProduct(B, vStress, dV);
		}
	}        
	FloatArray junk;
	junk.beProductOf(invH,h_int);
	answer.beTProductOf(Gamma,junk);
	answer.negated();
}

void
EnhancedAssumedStrainElementExtensionInterface ::computeEnhancedDisplacementVector(FloatArray &answer, StructuralElement *elem, TimeStep* tStep)
{
	FloatArray deltaDisplacement, deltaEnhancedDisplacement, h_int;
	FloatMatrix Gamma, invH;
	//this->giveInternalForcesCorrectionVector(h_int);
	this->giveTempInvStiffnessMatrixH(invH);
	this->giveTempStiffnessMatrixGamma(Gamma);
	elem->computeVectorOf(EID_MomentumBalance, VM_Incremental, tStep, deltaDisplacement);
	FloatArray junk;
	junk.beProductOf(Gamma,deltaDisplacement);
	deltaEnhancedDisplacement.beProductOf(invH,junk);
	//deltaEnhancedDisplacement.add(h_int);
	this->giveEnhancedDisplacementVector(answer);

	answer.subtract(deltaEnhancedDisplacement);

	
}



void
EnhancedAssumedStrainElementExtensionInterface :: updateYourself()
{
	enahncedDisplacementVector = tempEnahncedDisplacementVector;
	stiffnessMatrixGamma = tempStiffnessMatrixGamma;
	invStiffnessMatrixH = tempInvStiffnessMatrixH;
}




} // end namespace oofem
