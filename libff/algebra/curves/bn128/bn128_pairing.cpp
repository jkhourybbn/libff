/** @file
 ********************************************************************************
 Implements functions for computing Ate pairings over the bn128 curves, split into a
 offline and online stages.
 ********************************************************************************
 * @author     This file is part of libff, developed by SCIPR Lab
 *             and contributors (see AUTHORS).
 * @copyright  MIT license (see LICENSE file)
 *******************************************************************************/

#include <sstream>

#include <libff/algebra/curves/bn128/bn128_g1.hpp>
#include <libff/algebra/curves/bn128/bn128_g2.hpp>
#include <libff/algebra/curves/bn128/bn128_gt.hpp>
#include <libff/algebra/curves/bn128/bn128_init.hpp>
#include <libff/algebra/curves/bn128/bn128_pairing.hpp>
#include <libff/common/profiling.hpp>

namespace libff {

using std::size_t;

bool bn128_ate_G1_precomp::operator==(const bn128_ate_G1_precomp &other) const
{
    return (this->P[0] == other.P[0] &&
            this->P[1] == other.P[1] &&
            this->P[2] == other.P[2]);
}

std::ostream& operator<<(std::ostream &out, const bn128_ate_G1_precomp &prec_P)
{
    for (auto p : prec_P.P)
    {
#ifndef BINARY_OUTPUT
        out << p << "\n";
#else
        out.write((char*) &p, sizeof(p));
#endif
    }
    return out;
}

std::istream& operator>>(std::istream &in, bn128_ate_G1_precomp &prec_P)
{
    for (auto p : prec_P.P)
    {
#ifndef BINARY_OUTPUT
        in >> p;
        consume_newline(in);
#else
        in.read((char*) &p, sizeof(p));
#endif
    }
    return in;
}

bool bn128_ate_G2_precomp::operator==(const bn128_ate_G2_precomp &other) const
{
    if (!(this->Q[0] == other.Q[0] &&
          this->Q[1] == other.Q[1] &&
          this->Q[2] == other.Q[2] &&
          this->coeffs.size() == other.coeffs.size()))
    {
        return false;
    }

    /* work around for upstream serialization bug */
    for (size_t i = 0; i < this->coeffs.size(); ++i)
    {
        std::stringstream this_ss, other_ss;
        this_ss << this->coeffs[i];
        other_ss << other.coeffs[i];
        if (this_ss.str() != other_ss.str())
        {
            return false;
        }
    }

    return true;
}

std::ostream& operator<<(std::ostream &out, const bn128_ate_G2_precomp &prec_Q)
{
    for (auto q : prec_Q.Q)
    {
#ifndef BINARY_OUTPUT
        out << q.a_ << "\n";
        out << q.b_ << "\n";
#else
        out.write((char*) &q.a_, sizeof(q.a_));
        out.write((char*) &q.b_, sizeof(q.b_));
#endif
    }

    out << prec_Q.coeffs.size() << "\n";

    for (auto c : prec_Q.coeffs)
    {
#ifndef BINARY_OUTPUT
        out << c.a_.a_ << "\n";
        out << c.a_.b_ << "\n";
        out << c.b_.a_ << "\n";
        out << c.b_.b_ << "\n";
        out << c.c_.a_ << "\n";
        out << c.c_.b_ << "\n";
#else
        out.write((char*) &c.a_.a_, sizeof(c.a_.a_));
        out.write((char*) &c.a_.b_, sizeof(c.a_.b_));
        out.write((char*) &c.b_.a_, sizeof(c.b_.a_));
        out.write((char*) &c.b_.b_, sizeof(c.b_.b_));
        out.write((char*) &c.c_.a_, sizeof(c.c_.a_));
        out.write((char*) &c.c_.b_, sizeof(c.c_.b_));
#endif
    }

    return out;
}

std::istream& operator>>(std::istream &in, bn128_ate_G2_precomp &prec_Q)
{
    for (auto q : prec_Q.Q)
    {
#ifndef BINARY_OUTPUT
        in >> q.a_;
        consume_newline(in);
        in >> q.b_;
        consume_newline(in);
#else
        in.read((char*) &q.a_, sizeof(q.a_));
        in.read((char*) &q.b_, sizeof(q.b_));
#endif
    }

    size_t count;
    in >> count;
    consume_newline(in);
    prec_Q.coeffs.resize(count);
    for (size_t i = 0; i < count; ++i)
    {
#ifndef BINARY_OUTPUT
        in >> prec_Q.coeffs[i].a_.a_;
        consume_newline(in);
        in >> prec_Q.coeffs[i].a_.b_;
        consume_newline(in);
        in >> prec_Q.coeffs[i].b_.a_;
        consume_newline(in);
        in >> prec_Q.coeffs[i].b_.b_;
        consume_newline(in);
        in >> prec_Q.coeffs[i].c_.a_;
        consume_newline(in);
        in >> prec_Q.coeffs[i].c_.b_;
        consume_newline(in);
#else
        in.read((char*) &prec_Q.coeffs[i].a_.a_, sizeof(prec_Q.coeffs[i].a_.a_));
        in.read((char*) &prec_Q.coeffs[i].a_.b_, sizeof(prec_Q.coeffs[i].a_.b_));
        in.read((char*) &prec_Q.coeffs[i].b_.a_, sizeof(prec_Q.coeffs[i].b_.a_));
        in.read((char*) &prec_Q.coeffs[i].b_.b_, sizeof(prec_Q.coeffs[i].b_.b_));
        in.read((char*) &prec_Q.coeffs[i].c_.a_, sizeof(prec_Q.coeffs[i].c_.a_));
        in.read((char*) &prec_Q.coeffs[i].c_.b_, sizeof(prec_Q.coeffs[i].c_.b_));
#endif
    }
    return in;
}

bn128_ate_G1_precomp bn128_ate_precompute_G1(const bn128_G1& P)
{
    enter_block("Call to bn128_ate_precompute_G1");

    bn128_ate_G1_precomp result;
    bn::Fp P_coord[3];
    P.fill_coord(P_coord);
    bn::ecop::NormalizeJac(result.P, P_coord);

    leave_block("Call to bn128_ate_precompute_G1");
    return result;
}

bn128_ate_G2_precomp bn128_ate_precompute_G2(const bn128_G2& Q)
{
    enter_block("Call to bn128_ate_precompute_G2");

    bn128_ate_G2_precomp result;
    bn::Fp2 Q_coord[3];
    Q.fill_coord(Q_coord);
    bn::components::precomputeG2(result.coeffs, result.Q, Q_coord);

    leave_block("Call to bn128_ate_precompute_G2");
    return result;
}

bn128_Fq12 bn128_ate_miller_loop(const bn128_ate_G1_precomp &prec_P,
                                 const bn128_ate_G2_precomp &prec_Q)
{
    bn128_Fq12 f;
    bn::components::millerLoop(f.elem, prec_Q.coeffs, prec_P.P);
    return f;
}

bn128_Fq12 bn128_double_ate_miller_loop(const bn128_ate_G1_precomp &prec_P1,
                                        const bn128_ate_G2_precomp &prec_Q1,
                                        const bn128_ate_G1_precomp &prec_P2,
                                        const bn128_ate_G2_precomp &prec_Q2)
{
    bn128_Fq12 f;
    bn::components::millerLoop2(f.elem, prec_Q1.coeffs, prec_P1.P, prec_Q2.coeffs, prec_P2.P);
    return f;
}

bn128_GT bn128_final_exponentiation(const bn128_Fq12 &elt)
{
    enter_block("Call to bn128_final_exponentiation");
    bn128_GT eltcopy = elt;
    eltcopy.elem.final_exp();
    leave_block("Call to bn128_final_exponentiation");
    return eltcopy;
}
} // namespace libff
