#ifndef PREFILTERING_H
#define PREFILTERING_H

#include <iostream>
#include <fstream>
#include <unistd.h>
#include <string>
#include <time.h>
#include <sys/time.h>
#include <stddef.h>
#include <stdbool.h>

#include "DBReader.h"
#include "DBWriter.h"
#include "SubstitutionMatrix.h"
#include "Sequence.h"
#include "NucleotideMatrix.h"
#include "Debug.h"
#include "Log.h"
#include "Util.h"
#include "Parameters.h"

#include "ExtendedSubstitutionMatrix.h"
#include "ReducedMatrix.h"
#include "KmerGenerator.h"
#include "QueryTemplateMatcher.h"

#ifdef OPENMP
#include <omp.h>
#endif

class Prefiltering {

    public:

    Prefiltering(std::string queryDB,
                 std::string queryDBIndex,
                 std::string targetDB,
                 std::string targetDBIndex,
                 std::string outDB,
                 std::string outDBIndex,
                 Parameters par);

        ~Prefiltering();
        void run (size_t dbFrom,size_t dbSize,
                  std::string resultDB, std::string resultDBIndex);
        void run(int mpi_rank, int mpi_num_procs);
        void run ();
        void closeReader();
        void mergeOutput(std::vector<std::pair<std::string, std::string> > filenames);
        void removeDatabaes(std::vector<std::pair<std::string, std::string> > filenames);
        IndexTable * getIndexTable(int split, int splitCount); // needed for index lookup
    
        static IndexTable* generateIndexTable(DBReader* dbr, Sequence* seq,
                                         int alphabetSize, int kmerSize,
                                         size_t dbFrom, size_t dbTo, bool isLocal, int skip = 0);
    
    
        static void countKmersForIndexTable (DBReader* dbr, Sequence* seq, IndexTable* indexTable,
                                        size_t dbFrom, size_t dbTo);
    
        static void fillDatabase(DBReader* dbr, Sequence* seq, IndexTable * indexTable,
                                 size_t dbFrom, size_t dbTo);


        // get substituion matrix
        static BaseMatrix* getSubstitutionMatrix(std::string scoringMatrixFile, int alphabetSize, float bitFactor);


    private:
        static const size_t BUFFER_SIZE = 1000000;

        int threads;

        DBReader* qdbr;
        DBReader* tdbr;
        DBReader* tidxdbr;


        Sequence** qseq;
        int* notEmpty;

        std::list<int>** reslens;
        BaseMatrix* subMat;
        ExtendedSubstitutionMatrix* _2merSubMatrix;
        ExtendedSubstitutionMatrix* _3merSubMatrix;

        std::string outDB;
        std::string outDBIndex;
        // parameter
        int kmerSize;
        const int kmerScore;
        bool spacedKmer;
        const float sensitivity;
        const size_t maxResListLen;
        int alphabetSize;
        const float zscoreThr;
        const size_t maxSeqLen;
        const int querySeqType;
        const int targetSeqType;
        bool templateDBIsIndex;
    
        bool aaBiasCorrection;
        short kmerThr;
        double kmerMatchProb;
        int split;
        int skip;
        bool isLocal;
        // statistics
        size_t kmersPerPos;
        size_t resSize;
        size_t realResSize;
        size_t dbMatches;
        size_t doubleMatches;


        /* Set the k-mer similarity threshold that regulates the length of k-mer lists for each k-mer in the query sequence.
         * As a result, the prefilter always has roughly the same speed for different k-mer and alphabet sizes.
         */
        std::pair<short, double> setKmerThreshold(IndexTable *indexTable, DBReader *qdbr, DBReader *tdbr, float targetKmerMatchProb, double toleratedDeviation, int b);
        std::pair<std::string, std::string> createTmpFileNames(std::string db, std::string dbindex, int numb);
        // write prefiltering to ffindex database
        int writePrefilterOutput(DBWriter * dbWriter, int thread_idx, size_t id, std::pair<hit_t *,size_t> prefResults);
        // init QueryTemplateMatcher
        QueryTemplateMatcher **createQueryTemplateMatcher(BaseMatrix *m, IndexTable *indexTable, unsigned int *seqLens, short kmerThr, double kmerMatchProb, int kmerSize, size_t effectiveKmerSize, size_t dbSize, bool aaBiasCorrection, unsigned int maxSeqLen, float zscoreThr, bool isLocal, size_t maxHitsPerQuery);
    
    
        void printStatistics();

    statistics_t computeStatisticForKmerThreshold(IndexTable *indexTable, size_t querySetSize, unsigned int *querySeqsIds, bool reverseQuery, size_t kmerThrMid);
};

#endif
