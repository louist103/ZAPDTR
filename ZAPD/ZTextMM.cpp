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

	//ParseOoT();
	ParseMM();

	int bp2 = 0;
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
		msgEntry.icon = BitConverter::ToInt16BE(rawData, msgPtr + 2);
		msgEntry.nextMessageID = BitConverter::ToInt16BE(rawData, msgPtr + 4);
		msgEntry.firstItemCost = BitConverter::ToInt16BE(rawData, msgPtr + 6);
		msgEntry.secondItemCost = BitConverter::ToInt16BE(rawData, msgPtr + 8);

		msgEntry.segmentId = (codeData[langPtr + 4]);

		msgPtr += 11;

		unsigned char c = rawData[msgPtr];
		unsigned int extra = 0;
		bool stop = false;

		// Continue parsing until we are told to stop and all extra bytes are read
		while ((c != '\0' && !stop) || extra > 0)
		{
			msgEntry.msg += c;
			msgPtr++;

			// Some control codes require reading extra bytes
			if (extra == 0)
			{
				// End marker, so stop this message and do not read anything else
				if (c == 0x02)
				{
					stop = true;
				}
				else if (c == 0x05 || c == 0x13 || c == 0x0E || c == 0x0C || c == 0x1E ||
				         c == 0x06 || c == 0x14)
				{
					extra = 1;
				}
				// "Continue to new text ID", so stop this message and read two more bytes for the
				// text ID
				else if (c == 0x07)
				{
					extra = 2;
					stop = true;
				}
				else if (c == 0x12 || c == 0x11)
				{
					extra = 2;
				}
				else if (c == 0x15)
				{
					extra = 3;
				}
			}
			else
			{
				extra--;
			}

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
