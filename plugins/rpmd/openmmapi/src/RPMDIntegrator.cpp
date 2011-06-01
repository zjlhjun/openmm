/* -------------------------------------------------------------------------- *
 *                                   OpenMM                                   *
 * -------------------------------------------------------------------------- *
 * This is part of the OpenMM molecular simulation toolkit originating from   *
 * Simbios, the NIH National Center for Physics-Based Simulation of           *
 * Biological Structures at Stanford, funded under the NIH Roadmap for        *
 * Medical Research, grant U54 GM072970. See https://simtk.org.               *
 *                                                                            *
 * Portions copyright (c) 2008-2011 Stanford University and the Authors.      *
 * Authors: Peter Eastman                                                     *
 * Contributors:                                                              *
 *                                                                            *
 * Permission is hereby granted, free of charge, to any person obtaining a    *
 * copy of this software and associated documentation files (the "Software"), *
 * to deal in the Software without restriction, including without limitation  *
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,   *
 * and/or sell copies of the Software, and to permit persons to whom the      *
 * Software is furnished to do so, subject to the following conditions:       *
 *                                                                            *
 * The above copyright notice and this permission notice shall be included in *
 * all copies or substantial portions of the Software.                        *
 *                                                                            *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR *
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,   *
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL    *
 * THE AUTHORS, CONTRIBUTORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,    *
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR      *
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE  *
 * USE OR OTHER DEALINGS IN THE SOFTWARE.                                     *
 * -------------------------------------------------------------------------- */

#include "openmm/RPMDIntegrator.h"
#include "openmm/Context.h"
#include "openmm/OpenMMException.h"
#include "openmm/internal/ContextImpl.h"
#include "openmm/RPMDKernels.h"
#include <ctime>
#include <string>

using namespace OpenMM;
using std::string;
using std::vector;

RPMDIntegrator::RPMDIntegrator(int numCopies, double temperature, double frictionCoeff, double stepSize) : owner(NULL), numCopies(numCopies) {
    setTemperature(temperature);
    setFriction(frictionCoeff);
    setStepSize(stepSize);
    setConstraintTolerance(1e-4);
    setRandomNumberSeed((int) time(NULL));
}

void RPMDIntegrator::initialize(ContextImpl& contextRef) {
    if (owner != NULL && &contextRef.getOwner() != owner)
        throw OpenMMException("This Integrator is already bound to a context");
    context = &contextRef;
    owner = &contextRef.getOwner();
    kernel = context->getPlatform().createKernel(IntegrateRPMDStepKernel::Name(), contextRef);
    dynamic_cast<IntegrateRPMDStepKernel&>(kernel.getImpl()).initialize(contextRef.getSystem(), *this);
}

vector<string> RPMDIntegrator::getKernelNames() {
    std::vector<std::string> names;
    names.push_back(IntegrateRPMDStepKernel::Name());
    return names;
}

void RPMDIntegrator::setPositions(int copy, const vector<Vec3>& positions) {
    dynamic_cast<IntegrateRPMDStepKernel&>(kernel.getImpl()).setPositions(copy, positions);
}

void RPMDIntegrator::setVelocities(int copy, const vector<Vec3>& velocities) {
    dynamic_cast<IntegrateRPMDStepKernel&>(kernel.getImpl()).setVelocities(copy, velocities);
}

State RPMDIntegrator::getState(int copy, int types) {
    dynamic_cast<const IntegrateRPMDStepKernel&>(kernel.getImpl()).copyToContext(copy, *context);
    return context->getOwner().getState(types);
}

void RPMDIntegrator::step(int steps) {
    for (int i = 0; i < steps; ++i) {
        context->updateContextState();
        context->calcForcesAndEnergy(true, false);
        dynamic_cast<IntegrateRPMDStepKernel&>(kernel.getImpl()).execute(*context, *this);
    }
}
