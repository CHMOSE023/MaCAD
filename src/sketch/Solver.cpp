#include "sketch/Solver.hpp"

#include <algorithm>
#include <cmath>
#include <utility>
#include <vector>

namespace macad::sketch
{ 
    namespace 
    {

        // ---- small dense linear algebra (n is the free-variable count, small) -- 
        // Solves A x = b in place via Gauss-Newton with partial pivoting.
        // A is row-major n*n, b is n. Returns false if singular.
        bool solveDense(std::vector<double>& A, std::vector<double>& b, int n) 
        {
            for (int col = 0; col < n; ++col) {
                int pivot = col;
                double best = std::abs(A[col * n + col]);
                for (int r = col + 1; r < n; ++r) {
                    const double v = std::abs(A[r * n + col]);
                    if (v > best) { best = v; pivot = r; }
                }
                if (best < 1e-14) return false;
                if (pivot != col) {
                    for (int c = 0; c < n; ++c) std::swap(A[col * n + c], A[pivot * n + c]);
                    std::swap(b[col], b[pivot]);
                }
                const double diag = A[col * n + col];
                for (int r = 0; r < n; ++r) {
                    if (r == col) continue;
                    const double f = A[r * n + col] / diag;
                    if (f == 0.0) continue;
                    for (int c = col; c < n; ++c) A[r * n + c] -= f * A[col * n + c];
                    b[r] -= f * b[col];
                }
            }
            for (int i = 0; i < n; ++i) b[i] /= A[i * n + i];
            return true;
        }

        // ---- residual evaluation ----------------------------------------------

        // Read coordinates in double precision straight from the variable array.
        // (Sketch::pointPos truncates to float, which would swallow the ~1e-7
        // finite-difference perturbation and zero out the Jacobian.)
        glm::dvec2 pos(const Sketch& s, PointId id) 
        {
            const Point& p = s.point(id);
            return { s.var(p.vx), s.var(p.vy) };
        }

        // Direction (a, b) of a line entity as double-precision endpoints.
        void lineEnds(const Sketch& s, EntityId id, glm::dvec2& a, glm::dvec2& b) 
        {
            const Entity& e = s.entity(id);
            a = pos(s, e.p0);
            b = pos(s, e.p1);
        }

        double cross2(const glm::dvec2& u, const glm::dvec2& v) 
        {
            return u.x * v.y - u.y * v.x;
        }

        glm::dvec2 normalizeSafe(const glm::dvec2& v) 
        {
            const double len = std::sqrt(v.x * v.x + v.y * v.y);
            return len > 1e-12 ? glm::dvec2{ v.x / len, v.y / len } : glm::dvec2{ 1.0, 0.0 };
        }

        void appendResiduals(const Sketch& s, const Constraint& c, std::vector<double>& out) 
        {
            switch (c.kind) {
            case ConstraintKind::Coincident: {
                const auto a = pos(s, c.p0), b = pos(s, c.p1);
                out.push_back(a.x - b.x);
                out.push_back(a.y - b.y);
                break;
            }
            case ConstraintKind::Horizontal: {
                glm::dvec2 a, b; lineEnds(s, c.e0, a, b);
                out.push_back(a.y - b.y);
                break;
            }
            case ConstraintKind::Vertical: {
                glm::dvec2 a, b; lineEnds(s, c.e0, a, b);
                out.push_back(a.x - b.x);
                break;
            }
            case ConstraintKind::Parallel: {
                glm::dvec2 a0, b0, a1, b1;
                lineEnds(s, c.e0, a0, b0);
                lineEnds(s, c.e1, a1, b1);
                const auto d0 = normalizeSafe(b0 - a0);
                const auto d1 = normalizeSafe(b1 - a1);
                out.push_back(cross2(d0, d1));
                break;
            }
            case ConstraintKind::Tangent: {
                const Entity& e0 = s.entity(c.e0);
                const Entity& e1 = s.entity(c.e1);
                const bool c0 = e0.kind == EntityKind::Circle || e0.kind == EntityKind::Arc;
                const bool c1 = e1.kind == EntityKind::Circle || e1.kind == EntityKind::Arc;
                if (c0 && c1) {
                    // circle ~ circle: external tangency.
                    const auto ca = pos(s, e0.p0), cb = pos(s, e1.p0);
                    const double d = std::sqrt((ca.x - cb.x) * (ca.x - cb.x) +
                        (ca.y - cb.y) * (ca.y - cb.y));
                    out.push_back(d - (s.radius(c.e0) + s.radius(c.e1)));
                }
                else {
                    // line ~ circle: distance(center, line) == radius.
                    const EntityId lineId = c0 ? c.e1 : c.e0;
                    const EntityId circId = c0 ? c.e0 : c.e1;
                    glm::dvec2 a, b; lineEnds(s, lineId, a, b);
                    const auto ctr = pos(s, s.entity(circId).p0);
                    const auto d = normalizeSafe(b - a);
                    const double dist = std::abs(cross2(d, ctr - a));
                    out.push_back(dist - s.radius(circId));
                }
                break;
            }
            case ConstraintKind::Equal: {
                const Entity& e0 = s.entity(c.e0);
                const Entity& e1 = s.entity(c.e1);
                if (e0.kind == EntityKind::Line && e1.kind == EntityKind::Line) {
                    glm::dvec2 a0, b0, a1, b1;
                    lineEnds(s, c.e0, a0, b0);
                    lineEnds(s, c.e1, a1, b1);
                    const double l0 = std::sqrt((b0.x - a0.x) * (b0.x - a0.x) + (b0.y - a0.y) * (b0.y - a0.y));
                    const double l1 = std::sqrt((b1.x - a1.x) * (b1.x - a1.x) + (b1.y - a1.y) * (b1.y - a1.y));
                    out.push_back(l0 - l1);
                }
                else {
                    out.push_back(s.radius(c.e0) - s.radius(c.e1));
                }
                break;
            }
            case ConstraintKind::Distance: {
                const auto a = pos(s, c.p0), b = pos(s, c.p1);
                const double d = std::sqrt((b.x - a.x) * (b.x - a.x) + (b.y - a.y) * (b.y - a.y));
                out.push_back(d - c.value);
                break;
            }
            case ConstraintKind::Radius: {
                out.push_back(s.radius(c.e0) - c.value);
                break;
            }
            case ConstraintKind::Angle: {
                glm::dvec2 a0, b0, a1, b1;
                lineEnds(s, c.e0, a0, b0);
                lineEnds(s, c.e1, a1, b1);
                const auto d0 = normalizeSafe(b0 - a0);
                const auto d1 = normalizeSafe(b1 - a1);
                const double ang = std::atan2(cross2(d0, d1), d0.x * d1.x + d0.y * d1.y);
                out.push_back(ang - c.value);
                break;
            }
            }
        }

        std::vector<double> evalResiduals(const Sketch& s)
        {
            std::vector<double> r;
            for (const Constraint& c : s.constraints()) {
                if (c.removed) continue;
                appendResiduals(s, c, r);
            }
            return r;
        }

        double norm2(const std::vector<double>& v) 
        {
            double s = 0.0;
            for (double x : v) s += x * x;
            return s;
        }

    } // namespace

    SolveResult Solver::solve(Sketch& sketch, int maxIterations, double tolerance)
    {
        SolveResult result;

        // Collect free variable slots: coords of unfixed, live points + every
        // live circle/arc radius and arc angle.
        std::vector<int> freeIdx;
        for (const Point& p : sketch.points()) {
            if (p.removed || p.fixed) continue;
            freeIdx.push_back(p.vx);
            freeIdx.push_back(p.vy);
        }
        for (const Entity& e : sketch.entities()) {
            if (e.removed) continue;
            if (e.vr >= 0) freeIdx.push_back(e.vr);
            if (e.vStart >= 0) freeIdx.push_back(e.vStart);
            if (e.vEnd >= 0) freeIdx.push_back(e.vEnd);
        }
        const int n = static_cast<int>(freeIdx.size());

        std::vector<double> r = evalResiduals(sketch);
        const int m = static_cast<int>(r.size());
        result.freeVarCount = n;
        result.residualCount = m;
        result.dof = n - m;
        result.residualNorm = std::sqrt(norm2(r));

        if (m == 0) {
            result.converged = true;
            return result;
        }
        if (n == 0) {
            // Nothing to move; consistent only if residuals already vanish.
            result.converged = result.residualNorm < std::sqrt(tolerance);
            result.overConstrained = !result.converged;
            return result;
        }

        const double eps = 1e-7;
        double lambda = 1e-3;
        double prevCost = norm2(r);

        std::vector<double> J(static_cast<std::size_t>(m) * n, 0.0);
        std::vector<double> A(static_cast<std::size_t>(n) * n, 0.0);
        std::vector<double> g(n, 0.0);

        int iter = 0;
        for (; iter < maxIterations; ++iter) {
            if (std::sqrt(prevCost) < tolerance) break;

            // Finite-difference Jacobian: J(i, j) = d r_i / d x_j.
            for (int j = 0; j < n; ++j) {
                const int slot = freeIdx[j];
                const double orig = sketch.var(slot);
                sketch.setVar(slot, orig + eps);
                const std::vector<double> rp = evalResiduals(sketch);
                sketch.setVar(slot, orig);
                for (int i = 0; i < m; ++i) {
                    J[static_cast<std::size_t>(i) * n + j] = (rp[i] - r[i]) / eps;
                }
            }

            // Normal equations: g = J^T r, A = J^T J.
            for (int a = 0; a < n; ++a) {
                double gi = 0.0;
                for (int i = 0; i < m; ++i) gi += J[static_cast<std::size_t>(i) * n + a] * r[i];
                g[a] = gi;
                for (int b = a; b < n; ++b) {
                    double v = 0.0;
                    for (int i = 0; i < m; ++i)
                        v += J[static_cast<std::size_t>(i) * n + a] * J[static_cast<std::size_t>(i) * n + b];
                    A[static_cast<std::size_t>(a) * n + b] = v;
                    A[static_cast<std::size_t>(b) * n + a] = v;
                }
            }

            // Levenberg-Marquardt step with backtracking on lambda.
            bool stepped = false;
            for (int attempt = 0; attempt < 8; ++attempt) {
                std::vector<double> M = A;
                std::vector<double> rhs(n);
                for (int a = 0; a < n; ++a) {
                    M[static_cast<std::size_t>(a) * n + a] += lambda * (A[static_cast<std::size_t>(a) * n + a] + 1e-9);
                    rhs[a] = -g[a];
                }
                if (!solveDense(M, rhs, n)) {
                    lambda *= 4.0;
                    continue;
                }
                // Trial update.
                std::vector<double> saved(n);
                for (int a = 0; a < n; ++a) {
                    saved[a] = sketch.var(freeIdx[a]);
                    sketch.setVar(freeIdx[a], saved[a] + rhs[a]);
                }
                std::vector<double> rNew = evalResiduals(sketch);
                const double cost = norm2(rNew);
                if (cost < prevCost) {
                    r = std::move(rNew);
                    prevCost = cost;
                    lambda = std::max(lambda * 0.5, 1e-12);
                    stepped = true;
                    break;
                }
                // Reject: restore and increase damping.
                for (int a = 0; a < n; ++a) sketch.setVar(freeIdx[a], saved[a]);
                lambda *= 4.0;
            }
            if (!stepped) break; // cannot improve further
        }

        result.iterations = iter;
        result.residualNorm = std::sqrt(prevCost);
        result.converged = result.residualNorm < std::sqrt(tolerance) * 10.0;
        result.overConstrained = (m > n) || !result.converged;
        return result;
    } 

}  

