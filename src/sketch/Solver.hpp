#pragma once

// Numerical constraint solver. Treats the sketch as a vector of free unknowns
// (the non-fixed slots of Sketch::m_vars) and drives all constraint residuals to
// zero with damped Gauss-Newton (Levenberg-Marquardt). The Jacobian is computed
// by finite differences, so every constraint only has to supply a residual -- no
// hand-derived derivatives -- which keeps the constraint set cheap to extend.

#include "sketch/Sketch.hpp"

namespace macad::sketch 
{ 
    struct SolveResult 
    {
        bool   converged{ false };
        int    iterations{ 0 };
        double residualNorm{ 0.0 };

        // Degree-of-freedom hint: freeVars - residualEquations. >0 under-constrained,
        // 0 fully constrained, <0 over-constrained. Naive (counts redundant
        // constraints), so it is a guidance hint, not a proof.
        int dof{ 0 };
        int freeVarCount{ 0 };
        int residualCount{ 0 };
        bool overConstrained{ false };
    };

    class Solver 
    {
    public:
        // Solves in place, updating the sketch's variables. Fixed points are held.
        static SolveResult solve(Sketch& sketch, int maxIterations = 100, double tolerance = 1e-9);
    };

} 

