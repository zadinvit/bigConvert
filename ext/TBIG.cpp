/*!**************************************************************************
\file    TBIF.cpp
\author  Michal Havlicek, Jiri Filip, Radomir Vavra
\date    2017/09/21
\version 2.37

BIG data format for multiple image sequences encapsulation:

features:	- fast data loading to memory
			- fast memory access or seeking from HDD (if stored data exceeds memory)
application:	- BTF data
				- dynamic textures

TBIG.cpp - source file with encoding and decoding procedures

******************************************************************************/

//#define linux

#include "TBIG.h"

#include <cassert>

//=====================================================================================
// public
//=====================================================================================

TBIG::TBIG()
{
	data=0;
	map=0;
	message=0;
	name=0;
	text=0;
	xml=0;
	begins=0;
	angles_device=0;
	angles_ideal=0;
	angles_real=0;
	source=0;
}//--- TBIG ------------------------------------------------------------

TBIG::~TBIG()
{
	init();
}//--- ~TBIG ------------------------------------------------------------


int
	TBIG::load(const char * bigname, bool usemem, long long memlimit)
{
	init();
	if(bigname)
	{
		name=(char*)malloc(strlen(bigname)+1);
		sprintf(name,"%s",bigname);
		source=fopen(bigname,"rb");
		if(!source)
		{
			error("file not found","load");
			return -1;
		}
		long long id=0,load=0,size=0;
		#ifdef linux
		fseek(source,0,SEEK_END);
		long long eof=ftell(source);
		fseek(source,0,SEEK_SET);
		#else
		_fseeki64(source,0,SEEK_END);
		long long eof=_ftelli64(source);
		_fseeki64(source,0,SEEK_SET);
		#endif

printf("load(): loading %lld bytes\n",eof);

		#ifdef linux
		while(ftell(source)<eof)
		#else
		while(_ftelli64(source)<eof)
		#endif
		{
			if(fread(&id,longlongsize,1,source)!=1)
			{
				error("could not read chunk id","load");
				fclose(source);
				source=0;
				return -2;
			}

#ifdef linux
printf("load(): id=%lld @ %lld\n",id,ftell(source));
#else
printf("load(): id=%lld @ %lld\n",id,_ftelli64(source));
#endif

			if(fread(&size,longlongsize,1,source)!=1)
			{
				error("could not read chunk size","load");
				fclose(source);
				source=0;
				return -3;
			}
			if(size<0)
			{
				error("found damaged chunk","load");
				fclose(source);
				source=0;
				return -4;
			}

			float rest=0.0;

printf("load(): size=%lld\n",size);

			switch(id)
			{
			case 1:
				if(fread(&width,longlongsize,1,source)!=1)
				{
					error("could not read width","load");
					fclose(source);
					source=0;
					return -5;
				}
printf("load(): width=%lld\n",width);
				if(width<1)
				{
					error("loaded non positive width","load");
					fclose(source);
					source=0;
					return -6;
				}
				if(images<1)
				{
					warning("unexpected chunk (width)","load");
				}
				break;
			case 2:
				if(fread(&height,longlongsize,1,source)!=1)
				{
					error("could not read height","load");
					fclose(source);
					source=0;
					return -7;
				}
printf("load(): height=%lld\n",height);
				if(height<1)
				{
					error("loaded non positive height","load");
					fclose(source);
					source=0;
					return -8;
				}
				if(images<1)
				{
					warning("unexpected chunk (height)","load");
				}
				break;
			case 3:
				if(fread(&spectra,longlongsize,1,source)!=1)
				{
					error("could not read spectra","load");
					fclose(source);
					source=0;
					return -9;
				}
printf("load(): spectra=%lld\n",spectra);
				if(spectra<1)
				{
					error("loaded non positive number of spectra","load");
					fclose(source);
					source=0;
					return -10;
				}
				if(images<1)
				{
					warning("unexpected chunk (number of spectra)","load");
				}
				break;
			case 4:
				if(fread(&images,longlongsize,1,source)!=1)
				{
					error("could not read number of images","load");
					fclose(source);
					source=0;
					return -11;
				}
printf("load(): number of images: %lld\n",images);
				if(images<0)
				{
					error("strange number of images","load");
					fclose(source);
					source=0;
					return -12;
				}
				if(images<1)
				{
					note("no images expected","load");
				}
				break;
			case 5:
				if(fread(&type,longlongsize,1,source)!=1)
				{
					error("could not read type","load");
					fclose(source);
					source=0;
					return -13;
				}
printf("load(): data type: %lld\n",type);
				if((type!=-1)&&(type!=2)&&(type!=4))
				{
					warning("loaded not supported type","load");
					//fclose(source);
					//source=0;
					//return;
				}
				if(images<1)
				{
					warning("unexpected chunk (data type)","load");
				}
				break;
			case 6:
				if(images<1)
				{
					error("unexpected data (no images expected)","load");
					fclose(source);
					source=0;
					return -14;
				}
				#ifdef linux
				begin=ftell(source);
				#else
				begin=_ftelli64(source);
				#endif
				datasize=size;
printf("load(): data size: %lld @ %lld\n",datasize,begin);
                                if((usemem==true)&&((memlimit<0)||(datasize<=memlimit)))
                                {
					data=malloc(datasize);
					if(data)
					{
						if(fread(data,datasize,1,source)!=1)
						{
							error("could not read data","load");
							fclose(source);
							source=0;
							free(data);
							data=0;
							return -15;
						}
					} 
					else 
					{
						warning("allocation failed","load");
						#ifdef linux
						if(fseek(source,datasize,SEEK_CUR)!=0)
						#else
						if(_fseeki64(source,datasize,SEEK_CUR)!=0)
						#endif
						{
							error("could not jump over data","load");
							fclose(source);
							source=0;
							return -16;							
						}
						usemem=false;
					}
                                }
				else
				{ 
					#ifdef linux
					if(fseek(source,datasize,SEEK_CUR)!=0)
					#else
					if(_fseeki64(source,datasize,SEEK_CUR)!=0)
					#endif
					{
						error("could not jump over data","load");
						fclose(source);
						source=0;
						return -17;							
					}
				}
				break;
			case 7:
				if((type!=-1)||(images<1))
				{
					warning("unexpected map","load");
					#ifdef linux
					if(fseek(source,size,SEEK_CUR)!=0)
					#else
					if(_fseeki64(source,size,SEEK_CUR)!=0)
					#endif
					{
						error("could not jump over unexpected index map data","load");
						fclose(source);
						source=0;
						return -18;							
					}
				}
				else
				{
					map=(char*)malloc(size);
					if(map)
					{
						if(fread(map,size,1,source)!=1)
						{
							error("could not read index map","load");
							fclose(source);
							source=0;
							return -19;
						}
					}
					else
					{
						error("not enough memory to load index map","load");
						fclose(source);
						source=0;
						return -20;
					}
				}
				break;
			case 8:
				if(fread(&space,longlongsize,1,source)!=1)
				{
					error("could not load colour space","load");
					fclose(source);
					source=0;
					return -21;
				}
				if(images<1)
				{
					warning("unexpected chunk (space)","load");
				}
printf("load(): colour space: %lld\n",space);
					
				break;
			case 9:
				if((fread(&ppmm,floatsize,1,source)!=1)||
					(fread(&rest,floatsize,1,source)!=1))
				{
					error("could not load ppmm","load");
					fclose(source);
					source=0;
					return -22;
				}
				if(ppmm<0)
				{
					note("negative ppmm","load");
				}
				if(images<1)
				{
					warning("unexpected chunk (ppmm)","load");
				}
printf("load(): ppmm=%f\n",ppmm);
				break;
			case 10:
				if((fread(&dpi,floatsize,1,source)!=1)||
					(fread(&rest,floatsize,1,source)!=1))
				{
					error("could not load dpi","load");
					fclose(source);
					source=0;
					return -23;
				}
				if(dpi<0)
				{
					note("negative dpi","load");
				}
				if(images<1)
				{
					warning("unexpected chunk (dpi)","load");
				}
printf("load(): dpi=%f\n",dpi);
				break;

			case 14:
				if(images<1)
				{
					error("unexpected chunk (ideal angles)","load");
					fclose(source);
					source=0;
					return -24;
				}				
				angles_ideal=(float*)malloc(4*images*floatsize);
				if(angles_ideal)
				{
					if(fread(angles_ideal,4*images*floatsize,1,source)!=1)
					{
						error("unable to load ideal angles","load");
						fclose(source);
						source=0;
						return -25;
					}
				}
				else
				{
					error("unable to load ideal angles","load");
					fclose(source);
					source=0;
					return -26;
				}
				break;

			case 15:
				if(images<1)
				{
					error("unexpected chunk (real angles)","load");
					fclose(source);
					source=0;
					return -27;
				}				
				angles_real=(float*)malloc(4*images*floatsize);
				if(angles_real)
				{
					if(fread(angles_real,4*images*floatsize,1,source)!=1)
					{
						error("unable to load real angles","load");
						fclose(source);
						source=0;
						return -28;
					}
				}
				else
				{
					error("unable to load real angles","load");
					fclose(source);
					source=0;
					return -29;

				}
				break;

			case 16:
				if(images<1)
				{
					error("unexpected chunk (device angles)","load");
					fclose(source);
					source=0;
					return -30;
				}				
				angles_device=(float*)malloc(4*images*floatsize);
				if(angles_device)
				{
					if(fread(angles_device,4*images*floatsize,1,source)!=1)
					{
						error("unable to load device angles","load");
						fclose(source);
						source=0;
						return -31;
					}
				}
				else
				{
					error("unable to load device angles","load");
					fclose(source);
					source=0;
				}
				break;

			case 17:
				if(size<1)
				{
					error("non positive text length","load");
					fclose(source);
					source=0;
					return -32;
				}
				text=(char*)malloc(size);
				if(text)
				{
					if(fread(text,size,1,source)!=1)
					{
						error("unable to load text","load");
						fclose(source);
						source=0;
					}	
				}
				else
				{
					error("not enough memory to load text","load");
					fclose(source);
					source=0;
					return -33;
				}
				break;
			case 18:
				if(size<1)
				{
					error("non positive name length","load");
					fclose(source);
					source=0;
					return -34;
				}
				name=(char*)malloc(size);
				if(name)
				{
					if(fread(name,size,1,source)!=1)
					{
						error("unable to load name","load");
						fclose(source);
						source=0;
					}	
				}
				else
				{
					error("not enough memory to load name","load");
					fclose(source);
					source=0;
					return -35;
				}
				break;
			case 19:
				if(size<1)
				{
					error("non positive xml length","load");
					fclose(source);
					source=0;
					return -36;
				}
				xml=(char*)malloc(size);
				if(xml)
				{
					if(fread(xml,size,1,source)!=1)
					{
						error("unable to load xml","load");
						fclose(source);
						source=0;
					}	
				}
				else
				{
					error("not enough memory to load xml","load");
					fclose(source);
					source=0;
					return -37;
				}
				break;
			case 20:
				if(fread(&tiles,longlongsize,1,source)!=1)
				{
					error("could not load number of tiles","load");
					fclose(source);
					source=0;
					return -38;
				}
				if(tiles<1)
				{
					error("non positive number of tiles","load");
					fclose(source);
					source=0;
					return -39;
				}
				if(images<1)
				{
					warning("unexpected chunk (tiles)","load");
				}
				break;

			default:
				#ifdef linux
				if(fseek(source,size,SEEK_CUR)!=0)
				#else
				if(_fseeki64(source,size,SEEK_CUR)!=0)
				#endif
				{
					error("could not find next chunk","load");
					fclose(source);
					source=0;
					return -40;
				}
				break;
			}
		}

		if(test()<0)return -41;

		if(map!=0)
		{
			long long lastbegin=0;
			begins=(long long*)malloc(images*longlongsize);
			if(begins)
			{
				for(int i=0;i<images;i++)
				{
					begins[i]=lastbegin;
					if(map[i]==2)
					lastbegin+=(width*height*spectra*halffloatsize); else lastbegin+=(width*height*spectra*floatsize);
				}
			}
			else
			{
				error("not enough memory to precompute file positions (from map)","load");
				fclose(source);
				source=0;
				return -42;
			}				
		}
	}
	else 
	{
		error("not specified input","load");
		return -43;
	}
	return 0;
}//--- load ---------------------------------------------------------------

void 
	TBIG::get_pixel(int tile, int y, int x, int which, float RGB[])
{
    long long idx;
	unsigned short r;
	half h;
	if((type==2)||((type==-1)&&(map[which]==2)))
	{
		if(type==-1)idx=begins[which];
		else idx=which*tiles*height*width*spectra*halffloatsize+tile*height*width*spectra*halffloatsize+y*width*spectra*halffloatsize+x*spectra*halffloatsize;
		r=get_data_short(idx);
		h.setBits(r);
		RGB[0]=(float)h;
		r=get_data_short(idx+halffloatsize);
		h.setBits(r);
		RGB[1]=(float)h;
		r=get_data_short(idx+2*halffloatsize);
		h.setBits(r);
		RGB[2]=(float)h;
	}
	if((type==4)||((type==-1)&&(map[which]==4)))
	{
		if(type==-1)idx=begins[which];
		else idx=which*tiles*height*width*spectra*floatsize+tile*height*width*spectra*floatsize+y*width*spectra*floatsize+x*spectra*floatsize;
		RGB[0]=get_data_float(idx);
		RGB[1]=get_data_float(idx+floatsize);
		RGB[2]=get_data_float(idx+2*floatsize);
	}
}//--- get_pixel -----------------------------------------------------

//int
//	TBIG::save(char * filename, list<string>file_list, list<float>sangles1, list<float>sangles2, list<float>sangles3,
//	long long stiles, long long sspace, float sppmm, float sdpi, char * stextfile, char * sname, char * sxmlfile)
//{
//	init();
//	if(!filename)
//	{
//		error("unspecified output","save");
// 		return -1;
//	}
//	if(file_list.empty())
//	{
//		error("unspecified input file list","save");
//		return -2;
//	}
//	char * bigname=0;
//	char * bigsuffix=(char*)malloc(5);
//	if(!bigsuffix)
//	{
//		error("not enough memory to test file name","save");
//		return -3;
//	}
//	if(strlen(filename)>3)memcpy(bigsuffix,filename+strlen(filename)-4,4);
//	bigsuffix[4]=0;
//	if((!strcmp(bigsuffix,".big"))||(!strcmp(bigsuffix,".BIG")))
//	{
//		bigname=(char*)malloc(strlen(filename)+1);
//		if(!bigname)
//		{
//			free(bigsuffix);
//			error("not enough memory to update file name","save");
//			return -4;
//		}
//		sprintf(bigname,"%s",filename);
//	}
//	else
//	{
//		bigname=(char*)malloc(strlen(filename)+5);
//		if(!bigname)
//		{
//			free(bigsuffix);
//			error("not enough memory to upgrade file name","save");
//			return -5;
//		}
//		sprintf(bigname,"%s.big",filename);
//	}
//	free(bigsuffix);
//	FILE*big=fopen(bigname,"rb");
//	if(big)
//	{
//		error("file already exists","save");
//		fclose(big);
//		free(bigname);
//		return -6;
//	} 
//	big=fopen(bigname,"wb");
//	free(bigname);
//	if(!big)
//	{
//		error("could not create output file","save");
//  		return -7;
//	}
//	source=big;
//	long long id=0,value,l,loaded=0,nextid;
//	float fvalue=0.0;
//	long listindex=0;
//	images=file_list.size();
//	listindex=images;
//printf("images=%i\n",images);
//	if(write(big,4,8,(void*)(&images))<0)return -8;
//
//
//
//			long long ll=0;
//
//
//			ll=6;
//			if(fwrite(&ll,8,1,big)!=1)
//			{
//				error("cannot write data id","save");
//				return -8;
//			}
//			datasize=0; //
//
//			#ifdef linux
//printf("save(): saved id=6 @ %lld\n",ftell(big));
//			long int datasize_begin=ftell(big);
//			#else
//printf("save(): saved id=6 @ %lld\n",_ftelli64(big));
//			long int datasize_begin=_ftelli64(big);
//			#endif
//
//			ll=0;
//			if(fwrite(&ll,8,1,big)!=1)
//			{
//				error("cannot write data size","save");
//				return -9;
//			}
//
//			map=(char*)malloc(1);
//			if(!map)
//			{
//				error("not enough memory to save map","save");
//				return -11;
//			}
//			float***d=0;
//			char*dh=0;
//			int s,r,c;
//			long long fullfloats=0;
//			nextid=0;
//
//
//			listindex=0;
//
//			list<string>::iterator it;
//
//			for(it=file_list.begin();it!=file_list.end();++it)
//			{
//
//printf("[DEBUG] #%i including '%s' ... ",listindex+1,it->data());
//					if(read_OpenEXR_header(it->data(),&s,&r,&c)!=1)
//					{
//						error("not valid exr file","save");
//						if(d)freemem3(d,0,spectra-1,0,height-1,0,width-1);
//						return -12;
//					}
//printf("%ix%ix%i ",c,r,s);
//					if(!d)
//					{
//						d=allocation3(0,s-1,0,r-1,0,c-1);
//						if(!d)
//						{
//							error("not enough memory to load image","save");
//							return -13;
//						}
//						dh=(char*)malloc(s*r*c*halffloatsize);
//						if(!dh)
//						{
//							error("not enough memory to load image","save");
//							freemem3(d,0,spectra-1,0,height-1,0,width-1);
//							return -14;
//						}
//						spectra=(long long)s;
//						height=(long long)r;
//						width=(long long)c;
//
//					}
//					if((s!=spectra)||(r!=height)||(c!=width))
//					{
//     						if(s!=spectra)error("different spectra","save");
//						if(r!=height)error("different heights","save");
//						if(c!=width)error("different widths","save");
//						if(d)freemem3(d,0,spectra-1,0,height-1,0,width-1);
//						free(dh);
//						return -15;
//					}
//					if(isFloatEXR(it->data())==1)
//					{
//printf("(full-float)");
//						map[listindex]=4;
//						fullfloats++;
//						datasize+=spectra*height*width*floatsize;
//					}
//					else
//					{
//printf("(half-float)");
//						map[listindex]=2;
//						datasize+=spectra*height*width*halffloatsize;
//					}
//
//					if(map[listindex])
//					{
//						if(read_OpenEXR_file(it->data(),d,s,r,c)!=1)
//						{
//							error("could not read source file","save");
//							if(d)freemem3(d,0,spectra-1,0,height-1,0,width-1);
//							free(dh);
//							return -16;
//						}
//						for(int tr=0;tr<r;tr++)
//						for(int tc=0;tc<c;tc++)
//						for(int ts=0;ts<s;ts++)
//						{
//							if(fwrite(&(d[ts][tr][tc]),floatsize,1,big)!=1)
//							{
//								error("could not write","save");
//								if(d)freemem3(d,0,spectra-1,0,height-1,0,width-1);
//								free(dh);
//								return -17;
//							}
//						}
//					}
//					else
//					{
//						if(spectra!=3)
//						{
//							error("unsupported non three channel exr","save");
//							if(d)freemem3(d,0,spectra-1,0,height-1,0,width-1);
//							free(dh);
//							return -18;
//						}
//						if(read_OpenEXR_file_xyz_half(it->data(),dh,3,r,c)!=1)
//						{
//							error("could not read source file","save");
//							if(d)freemem3(d,0,spectra-1,0,height-1,0,width-1);
//							free(dh);
//							return -19;
//						}
//						if(fwrite(dh,width*height*spectra*halffloatsize,1,big)!=1)
//						{
//							error("could not write","save");
//							if(d)freemem3(d,0,spectra-1,0,height-1,0,width-1);
//							free(dh);
//							return -20;
//						}
//					}
//printf(" OK\n");
//					listindex++;
//					
//					map=(char*)realloc(map,listindex+1);
//					if(!map)
//					{
//						error("reallocation error","save");
//						freemem3(d,0,spectra-1,0,height-1,0,width-1);
//						free(dh);
//						return -21;
//					}
//				
//
//			}
//printf("processed %i images\n",images);
//			if(d)freemem3(d,0,spectra-1,0,height-1,0,width-1);
//			free(dh);
//			if((fullfloats==0)||(fullfloats==images))
//			{
//				free(map);
//				map=0;
//				if(fullfloats==0)type=2;
//				else type=4;
//			}
//			else
//			{
//				id=7;
//				ll=loaded;
//				if(write(big,7,images,map)<0)
//				{
//					error("could not save map","save");
//					return -22;
//				}
//				type=-1;	
//			}
//
//			#ifdef linux
//			long int lli=ftell(big);
//			if(fseek(big,datasize_begin,SEEK_SET)!=0)
//			#else
//			long int lli=_ftelli64(big);
//			if(_fseeki64(big,datasize_begin,SEEK_SET)!=0)
//			#endif
//			{
//				error("could not jump back to data","save");
//				return -23;
//			}
//#ifdef linux
//printf("write datasize (%lld) @%lld",datasize,ftell(big));
//#else
//printf("write datasize (%lld) @%lld",datasize,_ftelli64(big));
//#endif
//
//			if(fwrite(&datasize,8,1,big)!=1)
//			{
//				error("could not save data size","save");
//				return -24;
//			}
//			#ifdef linux
//			if(fseek(big,lli,SEEK_SET)!=0)
//			#else
//			if(_fseeki64(big,lli,SEEK_SET)!=0)
//			#endif
//			{
//				error("could not jump over data","save");
//				return -25;
//			}
//			if(write(big,5,8,(void*)(&type))<0)
//			{
//				printf("error: could not write data type info to output!\n");
//				return -26;
//			}
//			if(write(big,1,8,(void*)(&width))<0)
//			{
//				printf("error: could not write width info to output!\n");
//				return -27;
//			}
//			if(write(big,2,8,(void*)(&height))<0)
//			{
//				printf("error: could not write height info to output!\n");
//				return -28;
//			}
//			if(write(big,3,8,(void*)(&spectra))<0)
//			{
//				printf("error: could not write spectral info to output!\n");
//				return -29;
//			}
//			if((sspace==-1)||((sspace>0)&&(sspace<4)))
//			{
//				space=sspace;
//				if(write(big,8,8,(void*)(&space))<0)
//				{
//					printf("error: could not write colour space info to output!\n");
//					return -30;
//				}
//			}else warning("unsupported colour space","save");
//			if(sppmm>=0.0)
//			{
//				ppmm=sppmm;
//				if(write(big,9,4,(void*)(&ppmm))<0)
//				{
//					error("could not write ppmm info to output","save");
//					return -31;
//				}
//			}
//			if(sdpi>=0.0)
//			{
//				dpi=sppmm;
//				if(write(big,10,4,(void*)(&dpi))<0)
//				{
//					error("could not write dpi info to output","save");
//					return -32;
//				}
//			}
//			if(!sangles1.empty())
//			{
//				if(sangles1.size()!=4*images)
//				{
//					error("wrong number of real angle values","save");
//					return -33;
//				}
//				angles_real=(float*)malloc(sizeof(float)*images*4);
//				if(!angles_real)
//				{
//					error("not enough memory to save real angles","save");
//					return -34;
//				}
//				//memcpy(angles_real,sangles1,sizeof(float)*images*4);
//
//				list<float>::iterator itf;
//				int i=0;
//				for(itf=sangles1.begin();itf!=sangles1.end();++itf)
//				{
//					angles_real[i]=*itf;
//					i++;
//				}
//
//				if(write(big,14,sizeof(float)*images*4,(void*)angles_real)<0)
//				{
//					error("could not write real angles info to output","save");
//					return -35;
//				}
//			}
//			if(!sangles2.empty())
//			{
//				if(sangles2.size()!=4*images)
//				{
//					error("wrong number of ideal angle values","save");
//					return -36;
//				}
//				angles_ideal=(float*)malloc(sizeof(float)*images*4);
//				if(!angles_ideal)
//				{
//					error("not enough memory to save real angles","save");
//					return -37;
//				}
//
//				list<float>::iterator itf;
//				int i=0;
//				for(itf=sangles2.begin();itf!=sangles2.end();++itf)
//				{
//					angles_ideal[i]=*itf;
//					i++;
//				}
//
//				if(write(big,15,sizeof(float)*images*4,(void*)angles_ideal)<0)
//				{
//					error("could not write ideal angles info to output","save");
//					return -38;
//				}
//			}
//			if(!sangles3.empty())
//			{
//				if(sangles3.size()!=4*images)
//				{
//					error("wrong number of device angle values","save");
//					return -39;
//				}
//				angles_device=(float*)malloc(sizeof(float)*images*4);
//				if(!angles_device)
//				{
//					error("not enough memory to save device angles","save");
//					return -40;
//				}
//				//memcpy(angles_device,sangles3,sizeof(float)*images*4);
//
//				list<float>::iterator itf;
//				int i=0;
//				for(itf=sangles3.begin();itf!=sangles3.end();++itf)
//				{
//					angles_real[i]=*itf;
//					i++;
//				}
//
//				if(write(big,16,sizeof(float)*images*4,(void*)angles_device)<0)
//				{
//					error("could not write device angles info to output","save");
//					return -41;
//				}
//			}
//
//			if(stextfile)
//			{
//				if(include_file(stextfile,text)<0)return -42;
//				if(!text)note("skipped empty text file","save");
//				else
//				{
//					if(write(big,17,strlen(text),(void*)text)<0)
//					{
//						error("could not write text file to output","save");
//						return -43;
//					}
//				}
//			}
//			if(sname)
//			{
//				name=(char*)malloc(strlen(sname));
//				if(!name)
//				{
//					error("not enough memory to save material name","save");
//					return -44;
//				}
//				memcpy(name,sname,strlen(sname));
//				if(write(big,18,strlen(name),(void*)name)<0)
//				{
//					error("could not write material name to output","save");
//					return -45;
//				}
//			}
//			if(sxmlfile)
//			{
//				if(include_file(sxmlfile,xml)<0)return -43;
//				if(!text)note("skipped empty xml file","save");
//				else
//				{
//					if(write(big,19,strlen(xml),(void*)xml)<0)
//					{
//						error("could not write xml file to output","save");
//						return -46;
//					}
//				}
//			}
//			tiles=stiles;
//			if(images%stiles)
//			{
//				warning("incorrect number of tiles","save");
//				tiles=1;
//			}
//			if(write(big,20,8,(void*)(&tiles))<0)
//			{
//				error("could not write tiles info to ouput","save");
//				return -47;
//			}
//
//
//	//fclose(big);
//	return 0;
//}//--- save() -------------------------------------------------------------

//int TBIG::saveTXT(char * filename, char * listfile)
//{
//	int images=-1, aux;
//	char **listnames;
//	long long stiles=1; 
//	long long sspace=-1; 
//	float sppmm=-1.0; 
//	float sdpi=-1.0;
//	char * stextfile=0; 
//	char * sname=0;
//	char * sxmlfile=0;
//	int nameSize = 1000;
//
//	list<string>filelist;
//	string namestring;
//	list<float>sangles1;
//	list<float>sangles2;
//	list<float>sangles3;
//	float angle;
//	
//
//	// parsing TXT file -------------------------------------
//	FILE*script=fopen(listfile,"r");
//	if(!script)
//	{
//		error("could not open script file","save");
//		return -1;
//	}
//	int id = -1;
//
//	while(id!=6)
//	{
//		fscanf(script,"%d",&id);
//		switch(id)
//		{
//		case 4:
//			fscanf(script," %d\n",&images);
//			printf("id 04: images %d\n",images);
//			break;
//		case 5:
//			fscanf(script," %d\n",&aux);
//			printf("id 05: datatype %d\n",aux);
//			break;
//		case 8:
//			fscanf(script," %d\n",&sspace);
//			printf("id 08: color space %d\n",sspace);
//			break;
//		case 9:
//			fscanf(script," %f\n",&sppmm);
//			printf("id 09: ppmm %f\n",sppmm);
//			break;
//		case 10:
//			fscanf(script," %f\n",&sdpi);
//			printf("id 10: dpi %f\n",sdpi);
//			break;
//		case 17:
//			stextfile = (char*)calloc(nameSize,sizeof(char));
//			if (!stextfile) error("allocation failure (textfile)","saveTXT");
//			fscanf(script," %s\n",&stextfile);
//			printf("id 17: textfile %s\n",stextfile);
//			break;
//		case 18:
//			sname = (char*)calloc(nameSize,sizeof(char));
//			if (!sname) error("allocation failure (name)","saveTXT");
//			fscanf(script," %s\n",sname);
//			printf("id 18: name %s\n",sname);
//			break;
//		case 19:
//			sxmlfile = (char*)calloc(nameSize,sizeof(char));
//			if (!sname) error("allocation failure (xmlfile)","saveTXT");
//			fscanf(script," %s\n",&sxmlfile);
//			printf("id 19: xmlfile %d\n",sxmlfile);
//			break;
//		case 20:
//			fscanf(script," %d\n",&stiles);
//			printf("id 20: tiles %d\n",stiles);
//			break;
//		}
//	}// while id not 6
//
//    fscanf(script,"%d\n",&id);
//	if(id==6)
//	{
//		char * filename=(char*)malloc(nameSize);
//
//
//		printf("id 6: data start saving\n");
//		if(images<1)
//		{
//			printf("number of images: %d\n",images);
//			error("incorrect number loaded","saveTXT");
//		}
//		for(int i=0;i<images;i++)
//		{
//			fscanf(script,"%s ",filename);
//
//			namestring=filename;
//			filelist.push_back(namestring);
//
//			for(int k=0;k<4;k++)
//			{
//				fscanf(script,"%f ",&angle);
//				sangles1.push_back(angle);
//			}
//			for(int k=0;k<3;k++)
//			{
//				fscanf(script,"%f ",&angle);
//				sangles2.push_back(angle);
//			}
//			fscanf(script,"%f\n",&angle);
//			sangles2.push_back(angle);
//		}
//		printf("id 6: data saved\n");
//
//		free(filename);
//		namestring.clear();
//	}
//	fclose(script);
//
//
//	// saving data ------------------------------
//	save(filename, filelist, sangles1, sangles2, sangles3, stiles, sspace, sppmm, sdpi, stextfile, sname, sxmlfile);
//
//	// deallocations ----------------------------
//	sangles1.clear();
//	sangles2.clear();
//	sangles3.clear();
//	if(stextfile) free(stextfile);
//	if(sname) free(sname);
//	if(sxmlfile) free(sxmlfile);
//
//	filelist.clear();
//
//	return 0;
//}//--- saveTXT ----------------------------------------------------------

void
	TBIG::report()
{
	if(data)printf("data: %lli\n",data);
	if(map)
	{
		printf("map: %lli",map);
		if(begins)printf(" (begins: %lli)",begins);
		printf("\n");
	}
	if(message)printf("messages:\n %s\n",message);
	if(name)printf("name: %s\n",name);
	if(errors>0)printf("errors: %lli\n",errors);
	if(notes>0)printf("notes: %lli\n",notes);
	if(warnings>0)printf("warnings: %lli\n",warnings);
	if(begin>-1)printf("begin: %lli\n",begin);
	printf("datazise: %lli (bytes)\n",datasize);
	printf("height: %lli\n",height);
	if(images>0)printf("images: %lli\n",images);
	printf("%s\n",get_space());
	printf("spectra: %lli\n",spectra);
	if(tiles>0)printf("tiles: %lli\n",tiles);
	printf("%s\n",get_type());
	printf("width: %lli\n",width);
	printf("dpi: %f\n",dpi);
	printf("ppmm: %f\n",ppmm);
	if(angles_device)printf("angles_device: %lli\n",angles_device);
	if(angles_ideal)printf("angles_ideal: %lli\n",angles_ideal);
	if(angles_real)printf("angles_real: %lli\n",angles_real);
	if(source)printf("source: %lli\n",source);
}//--- report -----------------------------------------------------------------

void
	TBIG::get_angles(int index, float angles[])
{
	int i;
	for(i=0;i<12;i++)
	{
		angles[i]=0.0;
	}
	if((index>-1)&&(index<images-1))
	{
		for(i=0;i<4;i++)
		{
			if(angles_ideal)angles[i]=angles_ideal[4*index+i];
			if(angles_real)angles[i+4]=angles_real[4*index+i];
			if(angles_device)angles[i+8]=angles_device[4*index+i];
		}
	}
}//--- get_angles -----------------------------------------------------------

char * 
	TBIG::get_message()
{
	return message;
}//--- get_message ------------------------------------------------------------

char *
	TBIG::get_name()
{
	return name;
}//--- get_name() ------------------------------------------------------------

char *
	TBIG::get_space()
{
	switch(space)
	{
		case -1: return "unknown color space";
		case 1: return "sRGB";
		case 2: return "RGB";
		case 3: return "XZY";
	}
	return "unsupported or unknown data";
}//--- get_space() ------------------------------------------------------------

char * 
	TBIG::get_text()
{
	return text;
}//--- get_text() ------------------------------------------------------------

char *
	TBIG::get_type()
{
	switch(type)
	{
		case -1: return "mixed";
		case 2: return "half-float";
		case 4: return "full-float";
	}
	return "unsupported or unknown data type";
}//--- get_type() ------------------------------------------------------------

char * 
	TBIG::get_xml()
{
	return xml;
}//--- get_xml() -----------------------------------------------------------

int 
	TBIG::get_errors()
{
	return errors;
}//--- get_errors() -----------------------------------------------------------

int 
	TBIG::get_notes()
{
	return notes;
}//--- get_notes() ------------------------------------------------------------

int 
	TBIG::get_warnings()
{
	return warnings;
}//--- get_warnings() -----------------------------------------------------------

long long 
	TBIG::get_height()
{
	return height;
}//--- get_height -----------------------------------------------------------

long long 
	TBIG::get_images()
{
	return images;
}//--- get_images ------------------------------------------------------------

long long 
	TBIG::get_spectra()
{
	return spectra;
}//--- get_spectra -----------------------------------------------------------

long long 
	TBIG::get_tiles()
{
	return tiles;
}//--- get_tiles() -----------------------------------------------------------

long long 
	TBIG::get_width()
{
	return width;
}//--- get_width ------------------------------------------------------------

float TBIG::get_dpi()
{
	return dpi;
}//--- get_dpi() ------------------------------------------------------------

float 
	TBIG::get_ppmm()
{
	return ppmm;
}//--- get_ppmm() ------------------------------------------------------------

//=====================================================================================
// private
//=====================================================================================

void
	TBIG::error(char * message, char * position)
{
	set_message(message,position,errormessage);
}//--- error -----

void 
	TBIG::init()
{
	if(data)free(data);
	if(map)free(map);
	if(message)free(message);
	if(name)free(name);
	if(text)free(text);
	if(xml)free(xml);
	if(begins)free(begins);
	if(angles_device)free(angles_device);
	if(angles_ideal)free(angles_ideal);
	if(angles_real)free(angles_real);
	if(source)fclose(source);
	data=0;
	map=0;
	message=0;
	name=0;
	text=0;
	xml=0;
	errors=0;
	notes=0;
	warnings=0;
	begin=-1;
	datasize=0;
	height=0;
	images=0;
	space=-1;
	spectra=0;
	tiles=1;
	type=-2;
	width=0;
	begins=0;
	dpi=0.0;
	ppmm=0.0;
	angles_device=0;
	angles_ideal=0;
	angles_real=0;
	source=0;
}//--- init ------------------------------------------------------------

void
	TBIG::note(char * message, char * position)
{
	set_message(message,position,notemessage);
}//--- note ------------------------------------------------------------

void 
	TBIG::set_message(char * details, char * position, char type)
{
	if((details)||(position))
	{		
		int length=16;
		if(message)length+=strlen(message);
		char*pre=0;
		pre=(char*)malloc(length+1);
		if(!pre)
		{ 
			printf("not enough memory (set_message)\n"); 
			exit(0);
		}
		if(message)sprintf(pre,"%s",message);
		else sprintf(pre,"");
		if(message)free(message);
		switch(type)
		{
			case errormessage: 
				sprintf(pre,"%s\nerror: ",pre); 
				errors++;
				break;
			case warningmessage: 
				sprintf(pre,"%s\nwarning: ",pre); 
				warnings++;
				break;
			case notemessage: 
				sprintf(pre,"%s\nnote: ",pre); 
				notes++;
				break;
		}
		if(details)length+=strlen(details);
		if(position)length+=strlen(position);
		message=0;
		message=(char*)malloc(length+1);
		if(!message)
		{ 
			printf("not enough memory (set_message)"); 
			exit(0); 
		}
		if((details)&&(!position))sprintf(message,"%s%s\n",pre,details);
		if((!details)&&(position))sprintf(message,"%s(%s)\n",pre,position);
		if((details)&&(position))sprintf(message,"%s%s (%s)\n",pre,details,position);
		free(pre);
	}
}//--- set_message -----------------------------------------------------------

int 
	TBIG::test()
{
	if((data)&&(!source))note("data loaded in memory","test");
	if((!data)&&(source))note("data read from file","test");
	if((!data)&&(!source))
	{
		error("data not available","test");
		return -1;
	}
	if(images==0)note("no images","test");
	if(images>0)
	{
		if(width<1)
		{
			error("width value error or missing","test");
			return -2;
		}
		if(height<1)
		{
			error("height value error or missing","test");
			return -3;
		}
		if(spectra<1)
		{
			error("spectra value error or missing","test");
			return -4;
		}
		if(tiles<1)
		{
			error("error number of tiles","test");
			return -5;
		}
		if((type!=-1)&&(type!=2)&&(type!=4))warning("unknown data type","test");
		if(space==-1)note("unknown color space","test");
		if((space<-1)||(space>3))warning("unsupported color space","test");
		if(ppmm<=0.0)note("ppmm not present","test");
		if(dpi<=0.0)note("dpi not present","test");
		if(!angles_device)note("device angles not present","test");
		if(!angles_ideal)note("ideal angles not present","test");
		if(!angles_real)note("real angles not present","test");
	}
	if(!text)note("no text file included","test");
	if(!xml)note("no xml file included","test");
	return 0;
}//--- test ---------------------------------------------------------------

void
	TBIG::warning(char * message, char * position)
{
	set_message(message,position,warningmessage);
}//--- warning ------------------------------------------------------------

int
	TBIG::write(FILE * file, long long id, long long length, void * data)
{
	if(!file)
	{
 		error("null file handle","write");
		return -1;
	}
 	if(id<1)
 	{
		error("no useable id","write");
		return -2;
	}
	if(length<0)
	{
		error("no useable length","write");
		return -3;
	}
	if(length<1)
	{
  		warning("attempt to save empty chunk","write");
		return -4;
	}
	if(!data)
	{
		error("no data to save","write");
		return -5;
	}
	long long wid=id;
	if(fwrite(&wid,8,1,file)!=1)
	{
		error("cannot write id","write");
		return -6;
	}

#ifdef linux
printf("write(): id=%lld written @ %lld\n",wid,ftell(file));
#else
printf("write(): id=%lld written @ %lld\n",wid,_ftelli64(file));
#endif

	long long length8=length;
	while(length8%8!=0)length8++;
	if(fwrite(&length8,8,1,file)!=1)
	{
		error("cannot write length","write");
		return -7;
	}
	if(fwrite(data,length,1,file)!=1)
	{
		error("cannot write data","write");
		return -8;
	}
	while(length<length8)
	{
		char randombyte=0;
		if(fwrite(&randombyte,1,1,file)!=1)
		{
			error("cannot write to file","write");
			return -9;
		}
		length++;
	} 
printf("written %lld bytes\n",length8);
	return 0;
}//--- write -----------------------------------------------------------

unsigned short 
	TBIG::get_data_short(long long idx)
{
	if((idx<0)||(idx>=datasize))
	{
		error("index out of range","get_data_short");
		return -1.0;        
	}
	unsigned short a=0;
	if(data)
	{
		memcpy(&a,(char*)data+idx,halffloatsize);
		return a;
	}
	if(source)
	{
		idx+=begin;
		#ifdef linux
		if(fseek(source,idx,SEEK_SET)!=0)
		#else
		if(_fseeki64(source,idx,SEEK_SET)!=0)
		#endif
		{
			error("cannot find data","get_data_short");
			return a;
		}
		if(fread(&a,halffloatsize,1,source)!=1)
		{
			error("cannot read value","get_data_short");
		}
		return a;
	}
	note("no data","get_data_short");
	return a;
}//--- get_data_short ----------------------------------------------------------

float 
	TBIG::get_data_float(long long idx)
{
	if((idx<0)||(idx>=datasize))
	{
		error("index out of range","get_data_float");
		return -1.0;        
	}
        float aux=0.0;
	if(data)
	{
		memcpy(&aux,(char*)data+idx,floatsize);
		return aux;
	}
	if(source)
	{
		idx+=begin;
		#ifdef linux
		if(fseek(source,idx,SEEK_SET)!=0)
		#else
		if(_fseeki64(source,idx,SEEK_SET)!=0)
		#endif
		{
			error("cannot find data","get_data_float");
			return aux;
		}
		if(fread(&aux,floatsize,1,source)!=1)
		{
			error("cannot read value","get_data_float");
		}
		return aux;
	}
	note("no data","get_data_float");
	return aux;
}//--- get_data_float ----------------------------------------------------------

int
	TBIG::include_file(char * filename, char * text)
{
	FILE*attach=fopen(filename,"r");
	if(!attach)
	{
		error("could not open attach","save");
		return -1;
	}
	text=(char*)malloc(2);
	if(!text)
	{
		error("not enough memory to include external file","save");
		fclose(attach);
		return -2;
	}
	text[0]=0;
	char s;
	while(!feof(attach))
	{
		fscanf(attach,"%c",&s);
		if(!text[0])
		{
			memcpy(text,&s,1);
			text[1]=0;
		}
		else
		{
			long l=strlen(text);
			text=(char*)realloc(text,l+2);
			memcpy(text+l,&s,1);
			text[l+1]=0;
		}
	}
	fclose(attach);
	return 0;
}//--- include_file ----------------------------------------------------------

