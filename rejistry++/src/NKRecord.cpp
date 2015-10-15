/*
 *
 *  The Sleuth Kit
 *
 *  Contact: Brian Carrier [carrier <at> sleuthkit [dot] org]
 *  Copyright (c) 2010-2015 Basis Technology Corporation. All Rights
 *  reserved.
 *
 *  This software is distributed under the Common Public License 1.0
 *
 *  This is a C++ port of the Rejistry library developed by Willi Ballenthin.
 *  See https://github.com/williballenthin/Rejistry for the original Java version.
 */

/**
 * \file NKRecord.cpp
 *
 */

// Local includes
#include "NKRecord.h"
#include "REGFHeader.h"
#include "RejistryException.h"
#include "DirectSubkeyListRecord.h"
#include "EmptySubkeyList.h"

namespace Rejistry {

    const std::string NKRecord::MAGIC = "nk";

    NKRecord::NKRecord(RegistryByteBuffer * buf, uint32_t offset) : Record(buf, offset) {
        if (!(getMagic() == MAGIC)) {
            throw RegistryParseException("NKRecord magic value not found.");
        }
    }

    NKRecord::NKRecord(const NKRecord& sourceRecord) : Record(sourceRecord._buf, sourceRecord._offset) {        
    }

    bool NKRecord::hasClassname() const {
        return getDWord(CLASSNAME_OFFSET_OFFSET) != 0xFFFFFFFFL;
    }

    std::wstring NKRecord::getClassName() const {
        if (!hasClassname()) {
            return L"";
        }

        int32_t offset = (int32_t)getDWord(CLASSNAME_OFFSET_OFFSET);
        uint16_t length = getWord(CLASSNAME_LENGTH_OFFSET);
        uint32_t classnameOffset = REGFHeader::FIRST_HBIN_OFFSET + offset;
        std::auto_ptr< Cell > c(new Cell(_buf, classnameOffset));
        if (c.get() == NULL) {
            throw RegistryParseException("Failed to create cell for class name.");
        }

        std::vector<uint8_t> data = c->getData();
        if (length > data.size()) {
            throw RegistryParseException("Cell size insufficient for parsing classname.");
        }
        return std::wstring(data.begin(), data.end());
    }

    uint64_t NKRecord::getTimestamp() const {
        return getQWord(TIMESTAMP_OFFSET);
    }

    bool NKRecord::isRootKey() const {
        return (getWord(FLAGS_OFFSET) == 0x2C);
    }

    bool NKRecord::hasAsciiName() const {
        return (getWord(FLAGS_OFFSET) & 0x0020) == 0x0020;
    }

    std::wstring NKRecord::getName() const {
        uint32_t nameLength = getWord(NAME_LENGTH_OFFSET);
        if (hasAsciiName()) {
            // TODO: This is a little hacky but it should work fine
            // for ASCII strings.
            std::string name = getASCIIString(NAME_OFFSET, nameLength);
            return std::wstring(name.begin(), name.end());
        }

        return getUTF16String(NAME_OFFSET, nameLength);
    }

    bool NKRecord::hasParentRecord() const {
        if (isRootKey()) {
            return false;
        }

        try {
            std::auto_ptr< NKRecord > nkRecord(getParentRecord());
            return true;
        }
        catch (RegistryParseException& ) {
            return false;
        }
    }

    NKRecord::NKRecordPtr NKRecord::getParentRecord() const {
        int32_t offset = (int32_t)getDWord(PARENT_RECORD_OFFSET_OFFSET);
        uint32_t parentOffset = REGFHeader::FIRST_HBIN_OFFSET + offset;
        std::auto_ptr< Cell > c(new Cell(_buf, parentOffset));

        if (c.get() == NULL) {
            throw RegistryParseException("Failed to create Cell for parent.");
        }
        return c->getNKRecord();
    }

    uint32_t NKRecord::getNumberOfValues() const {
        uint32_t num = getDWord(VALUES_NUMBER_OFFSET);
        if (num == 0xFFFFFFFF) {
            return 0;
        }
        return num;
    }

    uint32_t NKRecord::getSubkeyCount() const {
        uint32_t num = getDWord(SUBKEY_NUMBER_OFFSET);
        if (num == 0xFFFFFFFF) {
            return 0;
        }
        return num;
    }

    SubkeyListRecord::SubkeyListRecordPtr NKRecord::getSubkeyList() const {
        if (getSubkeyCount() == 0) {
            return new EmptySubkeyList(_buf, 0);
        }

        uint32_t offset = (uint32_t)getDWord(SUBKEY_LIST_OFFSET_OFFSET);
        offset += REGFHeader::FIRST_HBIN_OFFSET;

        std::auto_ptr< Cell > c(new Cell(_buf, offset));

        if (c.get() == NULL) {
            throw RegistryParseException("Failed to create Cell for value list record.");
        }

        return c->getSubkeyList();
    }

    ValueListRecord::ValueListRecordPtr NKRecord::getValueList() const {
        if (getNumberOfValues() == 0) {
            return new ValueListRecord(_buf, 0, 0);
        }

        uint32_t offset = (uint32_t)getDWord(VALUE_LIST_OFFSET_OFFSET);
        offset += REGFHeader::FIRST_HBIN_OFFSET;

        std::auto_ptr< Cell > c(new Cell(_buf, offset));

        if (c.get() == NULL) {
            throw RegistryParseException("Failed to create Cell for value list record.");
        }

        return c->getValueListRecord(getNumberOfValues());
    }
};