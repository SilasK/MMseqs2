#ifndef QUERY_TEMPLATE_MATCHER_H
#define QUERY_TEMPLATE_MATCHER_H

#include <list>
#include <iostream>
#include <cstring>


#include "../commons/BaseMatrix.h"
#include "../commons/Sequence.h"
#include "ExtendedSubstitutionMatrix.h"
#include "QueryScore.h"
#include "IndexTable.h"
#include "KmerGenerator.h"


class QueryTemplateMatcher {
    public:
        QueryTemplateMatcher (ExtendedSubstitutionMatrix* _2merSubMatrix,
                ExtendedSubstitutionMatrix* _3merSubMatrix,
                IndexTable * indexTable,
                unsigned short * seqLens,
                short kmerThr,
                double kmerMatchProb,
                int kmerSize,
                int dbSize,
                int alphabetSize,
                int skip); 

        ~QueryTemplateMatcher();
        // returns result for the sequence
        std::list<hit_t>*  matchQuery (Sequence * seq);

    private:
        /* generates kmer lists */
        KmerGenerator * kmerGenerator;
        /* contains the sequences for a kmer */
        IndexTable * indexTable;
        /* calculates the score */
        QueryScore * queryScore;
        // k of the k-mer
        int kmerSize;
        // amino acid alphabet size
        int alphabetSize;
        // number of skipped k-mers in the sequence
        int skip;




};

#endif