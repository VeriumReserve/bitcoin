// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2017 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <pow.h>

#include <arith_uint256.h>
#include <chain.h>
#include <primitives/block.h>
#include <uint256.h>
#include <math.h>
#include <bignum.h>

/////////////////
extern arith_uint256 bnProofOfWorkLimit;
extern arith_uint256 bnProofOfWorkLimitTestNet;
static const int64_t nTargetTimespan = 2 * 60 * 60;  // 2 hours
double GetDifficulty(const CBlockIndex* blockindex = nullptr);

unsigned int calculateBlocktime(const CBlockIndex* pindex)
{
    unsigned int nBlockTime;
    double diff = GetDifficulty(pindex);
    double dBlockTime = -13.03*log(diff)+180;
    if (dBlockTime < 15)
    {
        nBlockTime = 15;
    }
    else
    {
        nBlockTime = dBlockTime;
    }
    return nBlockTime;
}

unsigned int GetNextTargetRequired(const CBlockIndex* pindexLast)
{
    CBigNum bnTargetLimit ( ArithToUint256( bnProofOfWorkLimit ) );
    if (pindexLast->nHeight <= 2)
        return bnTargetLimit.GetCompact(); // first few blocks
    const CBlockIndex* pindexPrev = pindexLast->pprev;
    const CBlockIndex* pindexPrevPrev = pindexPrev->pprev;
    unsigned int nTargetSpacing = calculateBlocktime(pindexPrev);
    int64_t nActualSpacing = pindexPrev->GetBlockTime() - pindexPrevPrev->GetBlockTime();
    int64_t targetTimespan;
    if (pindexLast->nHeight+1 > 2394)
    {
        targetTimespan = nTargetTimespan * 24;
    }
    else
    {
        targetTimespan = nTargetTimespan;
    }
    // ppcoin: target change every block
    // ppcoin: retarget with exponential moving toward target spacing
    CBigNum bnNew;
    bnNew.SetCompact(pindexPrev->nBits);
    int64_t nInterval = targetTimespan / nTargetSpacing;
    bnNew *= ((nInterval - 1) * nTargetSpacing + nActualSpacing + nActualSpacing);
    bnNew /= ((nInterval + 1) * nTargetSpacing);
    if (bnNew > bnTargetLimit)
    {
        bnNew = bnTargetLimit;
    }
    return bnNew.GetCompact();
}

bool CheckProofOfWork(uint256 hash, unsigned int nBits, const Consensus::Params& params)
{
    bool fNegative;
    bool fOverflow;
    arith_uint256 bnTarget;

    bnTarget.SetCompact(nBits, &fNegative, &fOverflow);

    // Check range
    if (fNegative || bnTarget == 0 || fOverflow || bnTarget > UintToArith256(params.powLimit))
        return false;

    // Check proof of work matches claimed amount
    if (UintToArith256(hash) > bnTarget)
        return false;

    return true;
}
