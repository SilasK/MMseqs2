//
// Created by mad on 3/24/15.
//

#ifndef _MMSEQS_PSSM_H_
#define _MMSEQS_PSSM_H_


#include <SubstitutionMatrix.h>
#include <stddef.h>
#include <sys/cdefs.h>
#include <zconf.h>
#include "MultipleAlignment.h"

class PSSMCalculator {
public:

    PSSMCalculator(SubstitutionMatrix *subMat, size_t maxSeqLength);

    ~PSSMCalculator();

    void computePSSMFromMSA(size_t setSize, size_t queryLength, const char **msaSeqs);
    void printProfile(size_t queryLength);
    void printPSSM(size_t queryLength);

private:
    SubstitutionMatrix * subMat;
    size_t maxSeqLength;

    // contains sequence weights (global)
    float * seqWeight;

    // contains MSA AA matchWeight
    float * matchWeight;

    // contains MSA AA pseudocount weight
    float * pseudocountsWeight;

    // Entropy of MSA
    float * Neff_M;

    // Profile of MSA
    float * profile;

    // PSSM contains log odds PSSM values
    char * pssm;

    // pseudocount matrix (mem aligned)
    float * R;

    // compute position-specific scoring matrix PSSM score
    // 1.) convert PFM to PPM (position probability matrix)
    //     Both PPMs assume statistical independence between positions in the pattern
    // 2.) PSSM Log odds score
    //     M_{aa,pos}={log(M_{aa,pos} / b_{aa}).
    void computeLogPSSM(char *pssm, float *profile, size_t queryLength, float scoreBias);


    // normalize a fector to 1.0
    float NormalizeTo1(float *array, int length, double const *def_array);

    // prepare pseudocounts
    void PreparePseudocounts(float *frequency, float *frequency_with_pseudocounts, size_t queryLength, const float *R);

    // compute the Neff_M per column -p log(p)
    void computeNeff_M(float *frequency, float *seqWeight, float *Neff_M, size_t queryLength, size_t setSize, char const **msaSeqs);

    // Compute weight for sequence based on "Position-based Sequence Weights' (1994)
    void computeSequenceWeights(float *seqWeight, size_t queryLength, size_t setSize, const char **msaSeqs);

    // compute pseudocounts from Neff_M -p log(p) per column
    void computePseudoCounts(float *profile, float *frequency, float *frequency_with_pseudocounts, size_t length);

    void computeMatchWeights(size_t setSize, size_t queryLength, const char **msaSeqs);
};


#endif //_MMSEQS_PSSM_H_