#include "ZTextMM.h"

#include "Globals.h"
#include "Utils/BitConverter.h"
#include <Utils/DiskFile.h>
#include "Utils/Path.h"
#include "Utils/StringHelper.h"
#include "ZFile.h"

REGISTER_ZFILENODE(TextMM, ZTextMM);

ZTextMM::ZTextMM(ZFile* nParent) : ZResource(nParent)
{
	RegisterRequiredAttribute("CodeOffset");
	RegisterOptionalAttribute("LangOffset", "0");
}

void ZTextMM::ParseRawData()
{
	ZResource::ParseRawData();

	ParseMM();
}

void ZTextMM::ParseMM()
{
	const auto& rawData = parent->GetRawData();
	uint32_t currentPtr = StringHelper::StrToL(registeredAttributes.at("CodeOffset").value, 16);
	uint32_t langPtr = currentPtr;
	bool isPalLang = false;

	if (StringHelper::StrToL(registeredAttributes.at("LangOffset").value, 16) != 0)
	{
		langPtr = StringHelper::StrToL(registeredAttributes.at("LangOffset").value, 16);

		if (langPtr != currentPtr)
			isPalLang = true;
	}

	std::vector<uint8_t> codeData;

	if (Globals::Instance->fileMode == ZFileMode::ExtractDirectory)
		codeData = Globals::Instance->GetBaseromFile("code");
	else
		codeData =
			Globals::Instance->GetBaseromFile(Globals::Instance->baseRomPath.string() + "code");

	while (true)
	{
		MessageEntryMM msgEntry;
		msgEntry.id = BitConverter::ToInt16BE(codeData, currentPtr + 0);
		msgEntry.msgOffset = BitConverter::ToInt32BE(codeData, langPtr + 4) & 0x00FFFFFF;

		uint32_t msgPtr = msgEntry.msgOffset;

		msgEntry.textboxType = (rawData[msgPtr + 0]);
		msgEntry.textboxYPos = (rawData[msgPtr + 1]);
		msgEntry.icon = (rawData[msgPtr + 2]);
		msgEntry.nextMessageID = BitConverter::ToInt16BE(rawData, msgPtr + 3);
		msgEntry.firstItemCost = BitConverter::ToInt16BE(rawData, msgPtr + 5);
		msgEntry.secondItemCost = BitConverter::ToInt16BE(rawData, msgPtr + 7);

		msgEntry.segmentId = (codeData[langPtr + 4]);

		msgPtr += 11;

		unsigned char c = rawData[msgPtr];

		// Read until end marker (0xBF)
		while (true) {
			msgEntry.msg += c;

			if (c == 0xBF) { // End marker
				break;
			}

			switch (c) {
				case 0x14: // Print: xx Spaces
					msgEntry.msg += rawData[msgPtr + 1];
					msgPtr++;
					break;
				case 0x1B: // Delay for xxxx Before Printing Remaining Text
				case 0x1C: // Keep Text on Screen for xxxx Before Closing
				case 0x1D: // Delay for xxxx Before Ending Conversation
				case 0x1E: // Play Sound Effect xxxx
				case 0x1F: // Delay for xxxx Before Resuming Text Flow
					msgEntry.msg += rawData[msgPtr + 1];
					msgEntry.msg += rawData[msgPtr + 2];
					msgPtr += 2;
					break;
			}

			msgPtr++;
			c = rawData[msgPtr];
		}

		messages.push_back(msgEntry);

		if (msgEntry.id == 0xFFFC || msgEntry.id == 0xFFFF)
			break;

		currentPtr += 8;

		if (isPalLang)
			langPtr += 4;
		else
			langPtr += 8;
	}
}

std::string ZTextMM::GetSourceTypeName() const
{
	return "u8";
}

size_t ZTextMM::GetRawDataSize() const
{
	return 1;
}

ZResourceType ZTextMM::GetResourceType() const
{
	return ZResourceType::TextMM;
}
