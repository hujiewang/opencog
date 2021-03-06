/*
 * opencog/learning/moses/scoring/behave_cscore.cc
 *
 * Copyright (C) 2002-2008 Novamente LLC
 * Copyright (C) 2012,2013 Poulin Holdings LLC
 * Copyright (C) 2014 Aidyia Limited
 * All Rights Reserved
 *
 * Written by Moshe Looks, Nil Geisweiller, Linas Vepstas
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License v3 as
 * published by the Free Software Foundation and including the exceptions
 * at http://opencog.org/wiki/Licenses
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program; if not, write to:
 * Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <opencog/comboreduct/crutil/exception.h>

#include "behave_cscore.h"

namespace opencog { namespace moses {

behavioral_score behave_cscore::get_bscore(const combo_tree& tr) const
{
    return _bscorer(tr);
}

composite_score behave_cscore::get_cscore(const combo_tree& tr)
{
    behavioral_score bs;
    try {
        bs = get_bscore(tr);
    }
    catch (combo::EvalException& ee)
    {
        // Exceptions are raised when operands are out of their
        // valid domain (negative input log or division by zero),
        // or outputs a value which is not representable (too
        // large exp or log). The error is logged as level fine
        // because this happens very often when learning continuous
        // functions, and it clogs up the log when logged at a
        // higher level.
        if (logger().isFineEnabled()) {
            logger().fine()
               << "The following candidate: " << tr << "\n"
               << "has failed to be evaluated, "
               << "raising the following exception: "
               << ee.get_message() << " " << ee.get_vertex();
        }
        return worst_composite_score;
    }
    score_t res = _ascorer(bs);

    complexity_t cpxy = _bscorer.get_complexity(tr);
    score_t cpxy_coef = _bscorer.get_complexity_coef();
    if (logger().isFineEnabled()) {
        logger().fine() << "behave_cscore: " << res
                        << " complexity: " << cpxy
                        << " cpxy_coeff: " << cpxy_coef;
    }

    return composite_score(res, cpxy, cpxy * cpxy_coef, 0.0);
}

composite_score behave_cscore::get_cscore(const behavioral_score& bs)
{
    score_t res = _ascorer(bs);

#if 0
XXX FIXME
    complexity_t cpxy = _bscorer.get_complexity(tr);
    score_t cpxy_coef = _bscorer.get_complexity_coef();
    if (logger().isFineEnabled()) {
        logger().fine() << "behave_cscore: " << res
                        << " complexity: " << cpxy
                        << " cpxy_coeff: " << cpxy_coef;
    }
    return composite_score(res, cpxy, cpxy * cpxy_coef, 0.0);
#endif

    return composite_score(res, 0, 0, 0.0);
}

score_t behave_cscore::best_possible_score() const
{
    return boost::accumulate(_bscorer.best_possible_bscore(), 0.0);
}


} // ~namespace moses
} // ~namespace opencog
