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
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "../sm/Materials/Micromorphic/micromorphicms.h"
#include "../sm/Materials/structuralmaterial.h"
#include "contextioerr.h"
#include "../sm/Elements/nlstructuralelement.h"
#include "gausspoint.h"

namespace oofem {
  MicromorphicMaterialStatus :: MicromorphicMaterialStatus(int n, Domain *d, GaussPoint *g, bool sym) : StructuralMaterialStatus(n, d, g), micromorphicVar(), micromorphicVarGrad(), micromorphicStress(), micromorphicStressGrad(), tempMicromorphicVar(), tempMicromorphicVarGrad(), tempMicromorphicStress(), tempMicromorphicStressGrad() 
{

    this->symmetryFlag = sym;
    int rsize = StructuralMaterial :: giveSizeOfVoigtSymVector( gp->giveMaterialMode() );
    strainVector.resize(rsize);
    stressVector.resize(rsize);
    // reset temp vars.
    tempStressVector = stressVector;
    tempStrainVector = strainVector;
    /// initialization of micromorphic vars

    micromorphicVar.resize(1);
    micromorphicVarGrad.resize(2);

    tempMicromorphicVar = micromorphicVar;
    tempMicromorphicVarGrad = micromorphicVarGrad;


    micromorphicStress.resize(1);
    micromorphicStressGrad.resize(2);

    tempMicromorphicStress = micromorphicStress;
    tempMicromorphicStressGrad = micromorphicStressGrad;

     
}


MicromorphicMaterialStatus :: ~MicromorphicMaterialStatus() { }


void MicromorphicMaterialStatus :: printOutputAt(FILE *File, TimeStep *tStep)
// Prints the strains and stresses on the data file.
{
    FloatArray helpVec;
    MaterialStatus :: printOutputAt(File, tStep);

    fprintf(File, "  strains ");
    //@todo here should be sym for microdil and without sym for cosserat!!
    if(symmetryFlag) {
      StructuralMaterial :: giveFullSymVectorForm( helpVec, strainVector, gp->giveMaterialMode() );
    } else {
      StructuralMaterial :: giveFullVectorForm( helpVec, strainVector, gp->giveMaterialMode() );
    }
    
    for ( auto &var : helpVec ) {
      fprintf( File, " %.4e", var );
    }
    
    fprintf(File, "\n              stresses");
    if(symmetryFlag) {
      StructuralMaterial :: giveFullSymVectorForm( helpVec, stressVector, gp->giveMaterialMode() );
    } else {
      StructuralMaterial :: giveFullVectorForm( helpVec, stressVector, gp->giveMaterialMode() );
    }
    
    for ( auto &var : helpVec ) {
      fprintf( File, " %.4e", var );
    }
    fprintf(File, "\n");

}




void MicromorphicMaterialStatus :: updateYourself(TimeStep *tStep)
// Performs end-of-step updates.
{
    StructuralMaterialStatus :: updateYourself(tStep);

    micromorphicVar = tempMicromorphicVar;
    micromorphicVarGrad = tempMicromorphicVarGrad;
    micromorphicStress = tempMicromorphicStress;    
    micromorphicStressGrad = tempMicromorphicStressGrad;    
}


void MicromorphicMaterialStatus :: initTempStatus()
//
// initialize record at the begining of new load step
//
{
    StructuralMaterialStatus :: initTempStatus();

    // see if vectors describing reached equilibrium are defined
    /* if ( this->giveStrainVector().giveSize() == 0 ) {
        strainVector.resize( StructuralMaterial :: giveSizeOfVoigtSymVector( gp->giveMaterialMode() ) );
    }

    if ( this->giveStressVector().giveSize() == 0 ) {
        stressVector.resize( StructuralMaterial :: giveSizeOfVoigtSymVector( gp->giveMaterialMode() ) );
	}*/

    // reset temp vars.
    tempMicromorphicVar = micromorphicVar;
    tempMicromorphicVarGrad = micromorphicVarGrad;
    tempMicromorphicStress = micromorphicStress;    
    tempMicromorphicStressGrad =  micromorphicStressGrad;
}


contextIOResultType
MicromorphicMaterialStatus :: saveContext(DataStream &stream, ContextMode mode, void *obj)
//
// saves full ms context (saves state variables, that completely describe
// current state)
{
    contextIOResultType iores;

    if ( ( iores = StructuralMaterialStatus :: saveContext(stream, mode, obj) ) != CIO_OK ) {
        THROW_CIOERR(iores);
    }


    return CIO_OK;
}


contextIOResultType
MicromorphicMaterialStatus :: restoreContext(DataStream &stream, ContextMode mode, void *obj)
//
// restores full material context (saves state variables, that completely describe
// current state)
//
{
    contextIOResultType iores;

    if ( ( iores = StructuralMaterialStatus :: restoreContext(stream, mode, obj) ) != CIO_OK ) {
        THROW_CIOERR(iores);
    }


    return CIO_OK;
}

  /*
void MicromorphicMaterialStatus :: copyStateVariables(const MaterialStatus &iStatus)
{
    MaterialStatus &tmpStat = const_cast< MaterialStatus & >(iStatus);
    const MicromorphicMaterialStatus &structStatus = dynamic_cast< MicromorphicMaterialStatus & >(tmpStat);

    strainVector = structStatus.giveStrainVector();
    stressVector = structStatus.giveStressVector();
    tempStressVector = structStatus.giveTempStressVector();
    tempStrainVector = structStatus.giveTempStrainVector();

    PVector = structStatus.givePVector();
    tempPVector = structStatus.giveTempPVector();
    CVector = structStatus.giveCVector();
    tempCVector = structStatus.giveTempCVector();
    FVector = structStatus.giveFVector();
    tempFVector = structStatus.giveTempFVector();
}

void MicromorphicMaterialStatus :: addStateVariables(const MaterialStatus &iStatus)
{
    printf("Entering MicromorphicMaterialStatus :: addStateVariables().\n");
}
  */
} // end namespace oofem

