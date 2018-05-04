#pragma once
#include <iostream>
#include <fstream>
#include <math.h>
#include <cstdlib>

#define MAP_X	    256				         
#define MAP_Z	    256				         
#define MAP_SCALE	20.0f		    

BITMAPINFOHEADER	landInfo;		
BITMAPINFOHEADER	waterInfo;				
unsigned char*      landTexture;	
unsigned int		land;
unsigned char*      waterTexture;	
unsigned int		water;
								
float terrain[MAP_X][MAP_Z][3];		
float sea[MAP_X][MAP_Z][3];						
									
unsigned char *LoadBitmapFile(char *filename, BITMAPINFOHEADER *bitmapInfoHeader){
	FILE                *filePtr;							  
	BITMAPFILEHEADER	bitmapFileHeader;		
	unsigned char		*bitmapImage;			
	int					imageIdx = 0;		  
	unsigned char		tempRGB;				
												
	filePtr = fopen(filename, "rb");
	if (filePtr == NULL)
		return NULL;

	fread(&bitmapFileHeader, sizeof(BITMAPFILEHEADER), 1, filePtr);
	fread(bitmapInfoHeader, sizeof(BITMAPINFOHEADER), 1, filePtr);
	fseek(filePtr, bitmapFileHeader.bfOffBits, SEEK_SET);
	bitmapImage = (unsigned char*)malloc(bitmapInfoHeader->biSizeImage);
	fread(bitmapImage, 1, bitmapInfoHeader->biSizeImage, filePtr);

	for (imageIdx = 0; imageIdx < bitmapInfoHeader->biSizeImage; imageIdx += 3)
	{
		tempRGB = bitmapImage[imageIdx];
		bitmapImage[imageIdx] = bitmapImage[imageIdx + 2];
		bitmapImage[imageIdx + 2] = tempRGB;
	}

	fclose(filePtr);
	return bitmapImage;
}

void LoadGrass()
{
	landTexture = LoadBitmapFile("grass.bmp", &landInfo);
	glGenTextures(1, &land);
	glBindTexture(GL_TEXTURE_2D, land);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGB, landInfo.biHeight, landInfo.biWidth, GL_RGB, GL_UNSIGNED_BYTE, landTexture);
}

void LoadWater() {
	waterTexture = LoadBitmapFile("water.bmp", &waterInfo);
	glGenTextures(1, &water);
	glBindTexture(GL_TEXTURE_2D, water);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	gluBuild2DMipmaps(GL_TEXTURE_2D, GL_RGB, waterInfo.biHeight, waterInfo.biWidth, GL_RGB, GL_UNSIGNED_BYTE, waterTexture);
}