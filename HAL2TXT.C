/*HAL Laboratory (GB) to MIDI converter*/
/*By Will Trowbridge*/
/*Portions based on code by ValleyBell*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stddef.h>

#define bankSize 16384

FILE* rom, * txt;
long bank;
long offset;
long tablePtrLoc;
long tableOffset;
int i, j;
char outfile[1000000];
int songNum;
long seqPtrs[4];
long songPtr;
int chanMask;
long bankAmt;
int foundTable = 0;
long firstPtr = 0;
int bankA = 0;
int tempPos = 0;
int format = 0;

unsigned static char* romData;
unsigned static char* exRomData;

const char MagicBytesA[6] = { 0xB8, 0xC8, 0x58, 0x16, 0x00, 0x21 };
const char MagicBytesB[4] = { 0x19, 0x19, 0x2A, 0x5F };
const char MagicBytesC[5] = { 0xC9, 0x58, 0x16, 0x00, 0x21 };
const char MagicBytesD[4] = { 0x4F, 0x06, 0x00, 0x21 };

/*Function prototypes*/
unsigned short ReadLE16(unsigned char* Data);
static void Write8B(unsigned char* buffer, unsigned int value);
static void WriteBE32(unsigned char* buffer, unsigned long value);
static void WriteBE24(unsigned char* buffer, unsigned long value);
static void WriteBE16(unsigned char* buffer, unsigned int value);
void song2txt(int songNum, long ptr);

/*Convert little-endian pointer to big-endian*/
unsigned short ReadLE16(unsigned char* Data)
{
	return (Data[0] << 0) | (Data[1] << 8);
}

static void Write8B(unsigned char* buffer, unsigned int value)
{
	buffer[0x00] = value;
}

static void WriteBE32(unsigned char* buffer, unsigned long value)
{
	buffer[0x00] = (value & 0xFF000000) >> 24;
	buffer[0x01] = (value & 0x00FF0000) >> 16;
	buffer[0x02] = (value & 0x0000FF00) >> 8;
	buffer[0x03] = (value & 0x000000FF) >> 0;

	return;
}

static void WriteBE24(unsigned char* buffer, unsigned long value)
{
	buffer[0x00] = (value & 0xFF0000) >> 16;
	buffer[0x01] = (value & 0x00FF00) >> 8;
	buffer[0x02] = (value & 0x0000FF) >> 0;

	return;
}

static void WriteBE16(unsigned char* buffer, unsigned int value)
{
	buffer[0x00] = (value & 0xFF00) >> 8;
	buffer[0x01] = (value & 0x00FF) >> 0;

	return;
}

int main(int args, char* argv[])
{
	printf("HAL Laboratory (GB) to TXT converter\n");
	if (args != 3)
	{
		printf("Usage: HAL2TXT <rom> <bank>\n");
		return -1;
	}
	else
	{
		if ((rom = fopen(argv[1], "rb")) == NULL)
		{
			printf("ERROR: Unable to open file %s!\n", argv[1]);
			exit(1);
		}
		else
		{
			bank = strtol(argv[2], NULL, 16);
			if (bank != 1)
			{
				bankAmt = bankSize;
			}
			else
			{
				bankAmt = 0;
			}
			fseek(rom, ((bank - 1) * bankSize), SEEK_SET);
			romData = (unsigned char*)malloc(bankSize);
			fread(romData, 1, bankSize, rom);

			/*Also copy data from bank 1 since some games store their song pointer table there*/
			fseek(rom, 0, SEEK_SET);
			exRomData = (unsigned char*)malloc(bankSize);
			fread(exRomData, 1, bankSize, rom);
			fclose(rom);

			/*Try to search the bank for song table loader - Method 1*/
			for (i = 0; i < bankSize; i++)
			{
				if ((!memcmp(&romData[i], MagicBytesA, 6)) && foundTable != 1)
				{
					tablePtrLoc = bankAmt + i + 6;
					printf("Found pointer to song table at address 0x%04x!\n", tablePtrLoc);
					tableOffset = ReadLE16(&romData[tablePtrLoc - bankAmt]);
					printf("Song table starts at 0x%04x...\n", tableOffset);
					foundTable = 1;
				}
			}

			/*Try to search the bank for song table loader - Method 2*/
			for (i = 0; i < bankSize; i++)
			{
				if ((!memcmp(&romData[i], MagicBytesB, 4)) && foundTable != 1)
				{
					tablePtrLoc = bankAmt + i - 2;
					printf("Found pointer to song table at address 0x%04x!\n", tablePtrLoc);
					tableOffset = ReadLE16(&romData[tablePtrLoc - bankAmt]);
					printf("Song table starts at 0x%04x...\n", tableOffset);
					foundTable = 1;
				}
			}

			/*Try to search the bank for song table loader - Method 3 - Adventures of Lolo*/
			for (i = 0; i < bankSize; i++)
			{
				if ((!memcmp(&exRomData[i], MagicBytesC, 5)) && foundTable != 1)
				{
					tablePtrLoc = i + 5;
					printf("Found pointer to song table at address 0x%04x!\n", tablePtrLoc);
					tableOffset = ReadLE16(&exRomData[tablePtrLoc]);
					printf("Song table starts at 0x%04x...\n", tableOffset);
					foundTable = 1;
					bankA = 1;
				}
			}

			/*Try to search the bank for song table loader - Method 4 - Ghostbusters 2*/
			for (i = 0; i < bankSize; i++)
			{
				if ((!memcmp(&romData[i], MagicBytesD, 4)) && foundTable != 1)
				{
					tablePtrLoc = bankAmt + i + 4;
					printf("Found pointer to song table at address 0x%04x!\n", tablePtrLoc);
					tableOffset = ReadLE16(&romData[tablePtrLoc - bankAmt]);
					printf("Song table starts at 0x%04x...\n", tableOffset);
					foundTable = 1;
					format = 1;
				}
			}
			
			/*Method 5 - Trax*/
			if (ReadLE16(&romData[0]) == 0x4299)
			{
				tableOffset = 0x4000;
				printf("Song table starts at 0x%04x...\n", tableOffset);
				foundTable = 1;
				format = 1;
			}



			if (foundTable == 1)
			{
				if (bankA == 0)
				{
					i = tableOffset - bankAmt;
				}
				else if (bankA == 1)
				{
					i = tableOffset;
				}

				firstPtr = ReadLE16(&romData[i]);
				songNum = 1;
				if (bankA == 0)
				{
					while (ReadLE16(&romData[i]) > bankSize && ReadLE16(&romData[i]) < (bankSize * 2))
					{
						songPtr = ReadLE16(&romData[i]);
						printf("Song %i: 0x%04X\n", songNum, songPtr);
						if (songPtr != 0)
						{
							song2txt(songNum, songPtr);
						}
						i += 2;
						songNum++;
					}
				}

				else if (bankA == 1)
				{
					while (ReadLE16(&exRomData[i]) > bankSize && ReadLE16(&exRomData[i]) < (bankSize * 2))
					{
						songPtr = ReadLE16(&exRomData[i]);
						printf("Song %i: 0x%04X\n", songNum, songPtr);
						if (songPtr != 0)
						{
							song2txt(songNum, songPtr);
						}
						i += 2;
						songNum++;
					}
				}

			}
			else
			{
				printf("ERROR: Magic bytes not found!\n");
				exit(-1);
			}

			printf("The operation was successfully completed!\n");
			return 0;
		}
	}
}

void song2txt(int songNum, long ptr)
{
	int chanNum = 0;
	int activeChan[4];
	int chanPtrs[4];
	int chanMask = 0;
	int noteVol = 0;
	int chanVol = 0;
	int chanSpeed = 0;
	int freq = 0;
	long romPos = 0;
	long seqPos = 0;
	int curTrack = 0;
	int seqEnd = 0;
	int curNote = 0;
	int curNoteLen = 0;
	int curInst = 0;
	long macroPos = 0;
	long macroRet = 0;
	int repeat = 0;
	long repeatPt = 0;
	long loop = 0;
	int k = 0;
	unsigned char command[4];
	int sfx = 0;
	sprintf(outfile, "song%i.txt", songNum);
	if ((txt = fopen(outfile, "wb")) == NULL)
	{
		printf("ERROR: Unable to write to file song%i.txt!\n", songNum);
		exit(2);
	}
	else
	{	
		for (k = 0; k < 4; k++)
		{
			chanPtrs[k] = 0;
		}
		
		if (format == 0)
		{
			/*Get information about each track*/
			romPos = ptr - bankAmt;
			chanNum = romData[romPos];
			fprintf(txt, "Number of active channels: %i\n\n", chanNum);
			romPos++;

			for (k = 0; k < chanNum; k++)
			{
				chanMask = romData[romPos + 2];
				switch (chanMask)
				{
				case 0x00:
					curTrack = 0;
					break;
				case 0x04:
					curTrack = 1;
					break;
				case 0x10:
					curTrack = 2;
					break;
				case 0x0C:
					curTrack = 3;
					break;
				default:
					curTrack = 0;
					break;
				}
				chanPtrs[curTrack] = ReadLE16(&romData[romPos]);
				romPos += 3;
			}

			for (curTrack = 0; curTrack < 4; curTrack++)
			{
				if (chanPtrs[curTrack] != 0)
				{
					fprintf(txt, "Channel %i: 0x%04X\n", (curTrack + 1), chanPtrs[curTrack]);
					seqPos = chanPtrs[curTrack];
					seqEnd = 0;

					while (seqEnd == 0)
					{
						command[0] = romData[seqPos - bankAmt];
						command[1] = romData[seqPos + 1 - bankAmt];
						command[2] = romData[seqPos + 2 - bankAmt];
						command[3] = romData[seqPos + 3 - bankAmt];

						/*Play note*/
						if (command[0] < 0xC0)
						{
							curNote = command[0];
							fprintf(txt, "Play note: %01X\n", curNote);
							seqPos++;
						}

						/*Play note (custom/manual length)*/
						else if (command[0] >= 0xC0 && command[0] < 0xE0)
						{
							curNote = command[0];
							curNoteLen = command[1];
							fprintf(txt, "Play note with custom length: %01X, length: %01X\n", curNote, curNoteLen);
							seqPos += 2;
						}

						/*Unknown command E1*/
						else if (command[0] == 0xE1)
						{
							fprintf(txt, "Unknown command E1: %01X\n", command[1]);
							seqPos += 2;
						}

						/*Unknown command E2*/
						else if (command[0] == 0xE2)
						{
							fprintf(txt, "Unknown command E2: %01X, %02X\n", command[1], command[2]);
							seqPos += 3;
						}

						/*Pitch slide*/
						else if (command[0] == 0xE3)
						{
							fprintf(txt, "Frequency slide: %01X\n", command[1]);
							seqPos += 2;
						}

						/*Set note volume*/
						else if (command[0] == 0xF0)
						{
							noteVol = command[1];
							fprintf(txt, "Set note volume: %01X\n", noteVol);
							seqPos += 2;
						}

						/*Set "echo" effect?*/
						else if (command[0] == 0xF1)
						{
							fprintf(txt, "Set echo effect?: %01X\n", command[1]);
							seqPos += 2;
						}

						/*Set channel speed*/
						else if (command[0] == 0xF2)
						{
							chanSpeed = command[1];
							fprintf(txt, "Set channel speed: %01X\n", chanSpeed);
							seqPos += 2;
						}

						/*Hold note*/
						else if (command[0] == 0xF3)
						{
							fprintf(txt, "Hold note: %01X\n", command[1]);
							seqPos += 2;
						}

						/*Set note size*/
						else if (command[0] == 0xF4)
						{
							fprintf(txt, "Set note size: %01X\n", command[1]);
							seqPos += 2;
						}

						/*Set frequency*/
						else if (command[0] == 0xF5)
						{
							freq = command[1];
							fprintf(txt, "Set frequency: %01X\n", freq);
							seqPos += 2;
						}

						/*Set instrument*/
						else if (command[0] == 0xF6)
						{
							curInst = command[1];
							fprintf(txt, "Set instrument: %01X\n", curInst);
							seqPos += 2;
						}

						/*Unknown command F7*/
						else if (command[0] == 0xF7)
						{
							fprintf(txt, "Unknown command F7: %01X\n", command[1]);
							seqPos += 2;
						}

						/*Go to loop point*/
						else if (command[0] == 0xF8)
						{
							loop = ReadLE16(&romData[seqPos - bankAmt + 1]);
							fprintf(txt, "Go to loop point: 0x%04X\n", loop);
							seqEnd = 1;
						}

						/*Unknown command F9*/
						else if (command[0] == 0xF9)
						{
							fprintf(txt, "Unknown command F9\n");
							seqPos++;
						}

						/*Play macro at position*/
						else if (command[0] == 0xFA)
						{
							macroPos = ReadLE16(&romData[seqPos - bankAmt + 1]);
							macroRet = seqPos + 3;
							fprintf(txt, "Play macro at position: 0x%04X\n", macroPos);
							seqPos += 3;
						}

						/*Return from macro*/
						else if (command[0] == 0xFB)
						{
							fprintf(txt, "Return from macro\n");
							seqPos++;
						}

						/*Repeat section # of times*/
						else if (command[0] == 0xFC)
						{
							repeat = command[1];
							repeatPt = seqPos + 2;
							fprintf(txt, "Repeat section %i times\n", repeat);
							seqPos += 2;
						}

						/*End of repeat section*/
						else if (command[0] == 0xFD)
						{
							fprintf(txt, "End of repeat section\n");
							seqPos++;
						}

						/*Set channel volume*/
						else if (command[0] == 0xFE)
						{
							chanVol = command[1];
							fprintf(txt, "Set channel volume: %i\n", chanVol);
							seqPos += 2;
						}

						/*End song without loop*/
						else if (command[0] == 0xFF)
						{
							fprintf(txt, "End of song, no loop\n");
							seqEnd = 1;
						}

						/*Unknown command*/
						else
						{
							fprintf(txt, "Unknown command: %01X\n", command[0]);
							seqPos++;
						}


					}
					fprintf(txt, "\n");
				}
			}
		}

		/*Ghostbusters 2 and Trax use a different song header format*/
		else if (format == 1)
		{
			romPos = ptr - bankAmt;

			while (romData[romPos] != 0xFF)
			{
				chanMask = romData[romPos];
				switch (chanMask)
				{
				case 0x00:
					curTrack = 0;
					sfx = 0;
					break;
				case 0x01:
					curTrack = 1;
					sfx = 0;
					break;
				case 0x02:
					curTrack = 2;
					sfx = 0;
					break;
				case 0x03:
					curTrack = 3;
					sfx = 0;
					break;
				case 0x04:
					curTrack = 0;
					sfx = 1;
					break;
				case 0x05:
					curTrack = 1;
					sfx = 1;
					break;
				case 0x06:
					curTrack = 2;
					sfx = 1;
					break;
				case 0x07:
					curTrack = 3;
					sfx = 1;
					break;
				}
				chanPtrs[curTrack] = ReadLE16(&romData[romPos + 1]);
				romPos += 3;
			}

			for (curTrack = 0; curTrack < 4; curTrack++)
			{
				if (chanPtrs[curTrack] != 0)
				{
					if (sfx == 0)
					{
						fprintf(txt, "Music channel %i: 0x%04X\n", (curTrack + 1), chanPtrs[curTrack]);
					}
					else if (sfx == 1)
					{
						fprintf(txt, "SFX channel %i: 0x%04X\n", (curTrack + 1), chanPtrs[curTrack]);
					}

					seqPos = chanPtrs[curTrack];
					seqEnd = 0;

					while (seqEnd == 0)
					{
						command[0] = romData[seqPos - bankAmt];
						command[1] = romData[seqPos + 1 - bankAmt];
						command[2] = romData[seqPos + 2 - bankAmt];
						command[3] = romData[seqPos + 3 - bankAmt];

						/*Play note*/
						if (command[0] < 0x80)
						{
							curNote = command[0];
							fprintf(txt, "Play note: %01X\n", curNote);
							seqPos++;
						}

						/*Change note length*/
						else if (command[0] >= 0x80 && command[0] < 0xE0)
						{
							curNoteLen = command[0];
							fprintf(txt, "Change note length: %01X\n", curNoteLen);
							seqPos++;
						}

						/*Unknown command E1*/
						else if (command[0] == 0xE1)
						{
							fprintf(txt, "Unknown command E1: %01X\n", command[1]);
							seqPos += 2;
						}

						/*Unknown command E2*/
						else if (command[0] == 0xE2)
						{
							fprintf(txt, "Unknown command E2: %01X, %02X\n", command[1], command[2]);
							seqPos += 3;
						}

						/*Unknown command E3*/
						else if (command[0] == 0xE3)
						{
							fprintf(txt, "Unknown command E3: %01X\n", command[1]);
							seqPos += 2;
						}

						/*Set note size*/
						else if (command[0] == 0xF0)
						{
							noteVol = command[1];
							fprintf(txt, "Set note size: %01X\n", noteVol);
							seqPos += 2;
						}

						/*Set duty*/
						else if (command[0] == 0xF1)
						{
							fprintf(txt, "Set duty: %01X\n", command[1]);
							seqPos += 2;
						}

						/*Set channel volume*/
						else if (command[0] == 0xF2)
						{
							noteVol = command[1];
							fprintf(txt, "Set note volume: %01X\n", noteVol);
							seqPos += 2;
						}

						/*Unknown command F3*/
						else if (command[0] == 0xF3)
						{
							fprintf(txt, "Hold note\n");
							seqPos++;
						}

						/*Repeat section # of times*/
						else if (command[0] == 0xF4)
						{
							repeat = command[1];
							repeatPt = seqPos + 2;
							fprintf(txt, "Repeat section %i times\n", repeat);
							seqPos += 2;
						}

						/*End of repeat*/
						else if (command[0] == 0xF5)
						{
							fprintf(txt, "End of repeat\n");
							seqPos++;
						}

						/*Play macro at position*/
						else if (command[0] == 0xF6)
						{
							macroPos = ReadLE16(&romData[seqPos - bankAmt + 1]);
							macroRet = seqPos + 3;
							fprintf(txt, "Play macro at position: 0x%04X\n", macroPos);
							seqPos += 3;
						}

						/*Return from macro*/
						else if (command[0] == 0xF7)
						{
							fprintf(txt, "Return from macro\n");
							seqPos += 2;
						}

						/*Pitch slide*/
						else if (command[0] == 0xF8)
						{
							fprintf(txt, "Pitch slide: %01X\n", command[1]);
							seqPos += 2;
						}

						/*Unknown command F9*/
						else if (command[0] == 0xF9)
						{
							fprintf(txt, "Unknown command F9\n");
							seqPos++;
						}

						/*Go to loop point*/
						else if (command[0] == 0xFB)
						{
							loop = ReadLE16(&romData[seqPos - bankAmt + 1]);
							fprintf(txt, "Go to loop point: 0x%04X\n", loop);
							seqEnd = 1;
						}					

						/*End song without loop*/
						else if (command[0] == 0xFF)
						{
							fprintf(txt, "End of song, no loop\n");
							seqEnd = 1;
						}

						/*Unknown command*/
						else
						{
							fprintf(txt, "Unknown command: %01X\n", command[0]);
							seqPos++;
						}
						

					}
					fprintf(txt, "\n");


				}
			}

			
		}

		fclose(txt);
	}
}