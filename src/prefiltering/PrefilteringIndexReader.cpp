#include "PrefilteringIndexReader.h"
#include "DBWriter.h"
#include "Prefiltering.h"
#include "ExtendedSubstitutionMatrix.h"
#include "FileUtil.h"

const char*  PrefilteringIndexReader::CURRENT_VERSION="3.2.0";
unsigned int PrefilteringIndexReader::VERSION = 0;
unsigned int PrefilteringIndexReader::META = 1;
unsigned int PrefilteringIndexReader::SCOREMATRIXNAME = 2;
unsigned int PrefilteringIndexReader::SCOREMATRIX2MER = 3;
unsigned int PrefilteringIndexReader::SCOREMATRIX3MER = 4;
unsigned int PrefilteringIndexReader::DBRINDEX = 5;

unsigned int PrefilteringIndexReader::ENTRIES = 91;
unsigned int PrefilteringIndexReader::ENTRIESOFFSETS = 92;
unsigned int PrefilteringIndexReader::ENTRIESNUM = 93;
unsigned int PrefilteringIndexReader::SEQCOUNT = 94;
unsigned int PrefilteringIndexReader::SEQINDEXDATA = 95;
unsigned int PrefilteringIndexReader::SEQINDEXDATASIZE = 96;
unsigned int PrefilteringIndexReader::SEQINDEXSEQOFFSET = 97;

bool PrefilteringIndexReader::checkIfIndexFile(DBReader<unsigned int>* reader) {
    char * version = reader->getDataByDBKey(VERSION);
    if(version == NULL){
        return false;
    }
    return (strncmp(version, CURRENT_VERSION, strlen(CURRENT_VERSION)) == 0 ) ? true : false;
}

void PrefilteringIndexReader::createIndexFile(std::string outDB, DBReader<unsigned int> *dbr,
                                              BaseMatrix * subMat, int maxSeqLen, bool hasSpacedKmer,
                                              bool compBiasCorrection, const int split, int alphabetSize, int kmerSize,
                                              bool diagonalScoring, bool maskResidues, int threads) {
    std::string outIndexName(outDB); // db.sk6
    std::string spaced = (hasSpacedKmer == true) ? "s" : "";
    outIndexName.append(".").append(spaced).append("k").append(SSTR(kmerSize));

    DBWriter writer(outIndexName.c_str(), std::string(outIndexName).append(".index").c_str(), 1, DBWriter::BINARY_MODE);
    writer.open();

    ScoreMatrix *s3 = ExtendedSubstitutionMatrix::calcScoreMatrix(*subMat, 3);
    char* serialized3mer = ScoreMatrix::serialize(*s3);
    Debug(Debug::INFO) << "Write SCOREMATRIX3MER (" << SCOREMATRIX3MER << ")\n";
    writer.writeData(serialized3mer, ScoreMatrix::size(*s3), SCOREMATRIX3MER, 0);
    free(serialized3mer);
    ScoreMatrix::cleanup(s3);

    writer.alignToPageSize();

    ScoreMatrix *s2 = ExtendedSubstitutionMatrix::calcScoreMatrix(*subMat, 2);
    char* serialized2mer = ScoreMatrix::serialize(*s2);
    Debug(Debug::INFO) << "Write SCOREMATRIX2MER (" << SCOREMATRIX2MER << ")\n";
    writer.writeData(serialized2mer, ScoreMatrix::size(*s2), SCOREMATRIX2MER, 0);
    free(serialized2mer);
    ScoreMatrix::cleanup(s2);

    Sequence seq(maxSeqLen, subMat->aa2int, subMat->int2aa, Sequence::AMINO_ACIDS, kmerSize, hasSpacedKmer, compBiasCorrection);

    for (int step = 0; step < split; step++) {
        size_t splitStart = 0;
        size_t splitSize  = 0;
        Util::decomposeDomainByAminoAcid(dbr->getAminoAcidDBSize(), dbr->getSeqLens(), dbr->getSize(),
                                         step, split, &splitStart, &splitSize);
        IndexTable *indexTable = new IndexTable(alphabetSize, kmerSize, false);
        Prefiltering::fillDatabase(dbr, &seq, indexTable, subMat, splitStart, splitStart + splitSize, diagonalScoring, maskResidues, 0, threads);
        indexTable->printStatistics(subMat->int2aa);


        // save the entries
        unsigned int entries_key = MathUtil::concatenate(ENTRIES, step);
        Debug(Debug::INFO) << "Write ENTRIES (" << entries_key << ")\n";
        char *entries = (char *) indexTable->getEntries();
        size_t entriesSize = indexTable->getTableEntriesNum() * indexTable->getSizeOfEntry();
        writer.writeData(entries, entriesSize, entries_key, 0);

        // save the size
        unsigned int entriesoffsets_key = MathUtil::concatenate(ENTRIESOFFSETS, step);
        Debug(Debug::INFO) << "Write ENTRIESOFFSETS (" << entriesoffsets_key << ")\n";

        char *offsets = (char*)indexTable->getOffsets();
        size_t offsetsSize = (indexTable->getTableSize() + 1) * sizeof(size_t);
        writer.writeData(offsets, offsetsSize, entriesoffsets_key, 0);
        indexTable->deleteEntries();

        SequenceLookup *lookup = indexTable->getSequenceLookup();
        unsigned int seqindexdata_key = MathUtil::concatenate(SEQINDEXDATA, step);
        Debug(Debug::INFO) << "Write SEQINDEXDATA (" << seqindexdata_key << ")\n";
        writer.writeData(lookup->getData(), lookup->getDataSize(), seqindexdata_key, 0);

        unsigned int seqindex_datasize_key = MathUtil::concatenate(SEQINDEXDATASIZE, step);
        Debug(Debug::INFO) << "Write SEQINDEXDATASIZE (" << seqindex_datasize_key << ")\n";
        int64_t seqindexDataSize = lookup->getDataSize();
        char *seqindexDataSizePtr = (char *) &seqindexDataSize;
        writer.writeData(seqindexDataSizePtr, 1 * sizeof(int64_t), seqindex_datasize_key, 0);

        size_t *sequenceOffsets = lookup->getOffsets();
        size_t sequenceCount = lookup->getSequenceCount();
        unsigned int seqindex_seqoffset = MathUtil::concatenate(SEQINDEXSEQOFFSET, step);
        Debug(Debug::INFO) << "Write SEQINDEXSEQOFFSET (" << seqindex_seqoffset << ")\n";
        writer.writeData((char *) sequenceOffsets, (sequenceCount + 1) * sizeof(size_t), seqindex_seqoffset, 0);

        // meta data
        // ENTRIESNUM
        unsigned int entriesnum_key = MathUtil::concatenate(ENTRIESNUM, step);
        Debug(Debug::INFO) << "Write ENTRIESNUM (" << entriesnum_key << ")\n";
        uint64_t entriesNum = indexTable->getTableEntriesNum();
        char *entriesNumPtr = (char *) &entriesNum;
        writer.writeData(entriesNumPtr, 1 * sizeof(uint64_t), entriesnum_key, 0);
        // SEQCOUNT
        unsigned int tablesize_key = MathUtil::concatenate(SEQCOUNT, step);
        Debug(Debug::INFO) << "Write SEQCOUNT (" << tablesize_key << ")\n";
        size_t tablesize = {indexTable->getSize()};
        char *tablesizePtr = (char *) &tablesize;
        writer.writeData(tablesizePtr, 1 * sizeof(size_t), tablesize_key, 0);

        delete indexTable;
    }
    Debug(Debug::INFO) << "Write META (" << META << ")\n";
    int local = 1;
    int spacedKmer = (hasSpacedKmer) ? 1 : 0;
    int metadata[] = {kmerSize, alphabetSize, maskResidues, split, local, spacedKmer};
    char *metadataptr = (char *) &metadata;
    writer.writeData(metadataptr, 6 * sizeof(int), META, 0);

    Debug(Debug::INFO) << "Write SCOREMATRIXNAME (" << SCOREMATRIXNAME << ")\n";
    writer.writeData(subMat->getMatrixName().c_str(), subMat->getMatrixName().length(), SCOREMATRIXNAME, 0);

    Debug(Debug::INFO) << "Write VERSION (" << VERSION << ")\n";
    writer.writeData((char *) CURRENT_VERSION, strlen(CURRENT_VERSION) * sizeof(char), VERSION, 0);

    Debug(Debug::INFO) << "Write DBRINDEX (" << DBRINDEX << ")\n";
    char* data = DBReader<unsigned int>::serialize(*dbr);
    writer.writeData(data, DBReader<unsigned int>::indexMemorySize(*dbr), DBRINDEX, 0);
    free(data);

    writer.close();
    Debug(Debug::INFO) << "Done. \n";
}

DBReader<unsigned int> *PrefilteringIndexReader::openNewReader(DBReader<unsigned int>*dbr) {
    char * data = dbr->getDataByDBKey(DBRINDEX);
    return DBReader<unsigned int>::unserialize(data);
}

SequenceLookup *PrefilteringIndexReader::getSequenceLookup(DBReader<unsigned int>*dbr, int split) {
    size_t sequenceCount    = *((size_t *)dbr->getDataByDBKey(MathUtil::concatenate(SEQCOUNT, split)));

    char * seqData = dbr->getDataByDBKey(MathUtil::concatenate(SEQINDEXDATA, split));
    size_t seqOffsetsId = dbr->getId(MathUtil::concatenate(SEQINDEXSEQOFFSET, split));
    char * seqOffsetsData = dbr->getData(seqOffsetsId);
    size_t seqOffsetLength = dbr->getSeqLens(seqOffsetsId);

    SequenceLookup *sequenceLookup = new SequenceLookup(sequenceCount);
    sequenceLookup->initLookupByExternalData(seqData, seqOffsetLength, (size_t *) seqOffsetsData);

    return sequenceLookup;
}

IndexTable *PrefilteringIndexReader::generateIndexTable(DBReader<unsigned int>*dbr, int split, bool diagonalScoring) {
    PrefilteringIndexData data = getMetadata(dbr);
    IndexTable *retTable;
    if (data.local) {
        retTable = new IndexTable(data.alphabetSize, data.kmerSize, true);
    }else {
        Debug(Debug::ERROR) << "Search mode is not valid.\n";
        EXIT(EXIT_FAILURE);
    }

    SequenceLookup * sequenceLookup = NULL;
    if(diagonalScoring == true) {
        sequenceLookup = getSequenceLookup(dbr, split);
    }

    int64_t entriesNum = *((int64_t *)dbr->getDataByDBKey(MathUtil::concatenate(ENTRIESNUM, split)));
    size_t sequenceCount = *((size_t *)dbr->getDataByDBKey(MathUtil::concatenate(SEQCOUNT, split)));

    char *entriesData = dbr->getDataByDBKey(MathUtil::concatenate(ENTRIES, split));
    char *entriesOffsetsData = dbr->getDataByDBKey(MathUtil::concatenate(ENTRIESOFFSETS, split));
    retTable->initTableByExternalData(sequenceCount, entriesNum,
                                      (IndexEntryLocal*) entriesData, (size_t *)entriesOffsetsData, sequenceLookup);
    return retTable;
}


PrefilteringIndexData PrefilteringIndexReader::getMetadata(DBReader<unsigned int>*dbr) {
    PrefilteringIndexData prefData;
    Debug(Debug::INFO) << "Index version: " << dbr->getDataByDBKey(VERSION) << "\n";
    int *metadata_tmp = (int *) dbr->getDataByDBKey(META);

    Debug(Debug::INFO) << "KmerSize:     " << metadata_tmp[0] << "\n";
    Debug(Debug::INFO) << "AlphabetSize: " << metadata_tmp[1] << "\n";
    Debug(Debug::INFO) << "Mask:         " << metadata_tmp[2] << "\n";
    Debug(Debug::INFO) << "Split:        " << metadata_tmp[3] << "\n";
    Debug(Debug::INFO) << "Type:         " << metadata_tmp[4] << "\n";
    Debug(Debug::INFO) << "Spaced:       " << metadata_tmp[5] << "\n";

    Debug(Debug::INFO) << "ScoreMatrix:  " << dbr->getDataByDBKey(SCOREMATRIXNAME) << "\n";

    prefData.kmerSize = metadata_tmp[0];
    prefData.alphabetSize = metadata_tmp[1];
    prefData.maskResidues = metadata_tmp[2];
    prefData.split = metadata_tmp[3];
    prefData.local = metadata_tmp[4];
    prefData.spacedKmer = metadata_tmp[5];

    return prefData;
}

std::string PrefilteringIndexReader::getSubstitutionMatrixName(DBReader<unsigned int> *dbr) {
    return std::string(dbr->getDataByDBKey(SCOREMATRIXNAME));
}
//
ScoreMatrix *PrefilteringIndexReader::get2MerScoreMatrix(DBReader<unsigned int> *dbr) {
    PrefilteringIndexData meta = getMetadata(dbr);
    return ScoreMatrix::unserialize(dbr->getDataByDBKey(SCOREMATRIX2MER), meta.alphabetSize, 2);
}

ScoreMatrix *PrefilteringIndexReader::get3MerScoreMatrix(DBReader<unsigned int> *dbr) {
    PrefilteringIndexData meta = getMetadata(dbr);
    return ScoreMatrix::unserialize(dbr->getDataByDBKey(SCOREMATRIX3MER), meta.alphabetSize, 3);
}

std::string PrefilteringIndexReader::searchForIndex(const std::string &pathToDB) {
    for (size_t spaced = 0; spaced < 2; spaced++) {
        for (size_t k = 5; k <= 7; k++) {
            std::string outIndexName(pathToDB); // db.sk6
            std::string s = (spaced == true) ? "s" : "";
            outIndexName.append(".").append(s).append("k").append(SSTR(k));
            if (FileUtil::fileExists(outIndexName.c_str()) == true) {
                return outIndexName;
            }
        }
    }

    return "";
}
