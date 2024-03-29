#include "filesystem.h"
#include "syscalls.h"
#include "graphics.h"

int dont_backup = 0;

int check_auto(string appfolder)
{
	dont_backup=exists((appfolder+"/data/backup.cfg").c_str());
	return dont_backup;
}

string int_to_string(int number)
{
	if (number == 0) return "0";
	string temp="";
	string returnvalue="";
	while (number>0)
	{
		temp+=number%10+48;
		number/=10;
	}
	for (size_t i=0;i<temp.length();i++)
		returnvalue+=temp[temp.length()-i-1];

	return returnvalue;
}

string convert_size(double size, string format)
{
	char str[100];

	if (format=="auto")
	{
		if (size >= 1073741824) format="GB";
		else if (size >= 1048576) format="MB";
		else format="KB";
	}
	if (format=="KB") size = size / 1024.00; // convert to KB
	else if (format=="MB") size = size / 1048576.00; // convert to MB
	else if (format=="GB") size = size / 1073741824.00; // convert to GB
	if (format=="KB") sprintf(str, "%.2fKB", size);
	else if (format=="MB") sprintf(str, "%.2fMB", size);
	else if (format=="GB") sprintf(str, "%.2fGB", size);

	return str;
}

double get_free_space(const char *path)
{
	uint32_t block_size;
	uint64_t free_block_count;

	sysFsGetFreeSize(path, &block_size, &free_block_count);
	return (((uint64_t) block_size * free_block_count));
}

double get_filesize(const char *path)
{
	sysFSStat info;

	if (sysFsStat(path, &info) >= 0) return (double)info.st_size;
	else return 0;
}

const string fileCreatedDateTime(const char *path)
{
	time_t tmod;
	char buf[80];
	sysFSStat info;

	if (sysFsStat(path, &info) >= 0)
	{
		tmod=info.st_mtime;
		strftime(buf, sizeof(buf), "%Y-%m-%d %Hh%Mm%Ss", localtime(&tmod));
		return buf;
	}
	else return "";
}

string create_file(const char* cpath)
{
  FILE *path;

  /* open destination file */
  if((path = fopen(cpath, "wb"))==NULL) return "Nao foi possivel abrir o arquivo ("+(string)cpath+") para escrever!";
  if(fclose(path)==EOF) return "Nao foi possivel fechar o arquivo ("+(string)cpath+")!";

  return "";
}

int is_dir(const char *path)
{
	sysFSStat info;

	if (sysFsStat(path, &info) >= 0)
		return ((info.st_mode & S_IFDIR) != 0);
	else
		return 0;
}

int exists(const char *path)
{
	sysFSStat info;

	if (sysFsStat(path, &info) >= 0) return 0;
	return -1;
}

int exists_backups(string appfolder)
{
	if(dont_backup) return -1;
	return exists((appfolder+"/backups").c_str());
}

int mkdir_one(string dirtocreate)
{
	return mkdir(dirtocreate.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
}

int mkdir_full(string fullpath)
{
	string pathtocreate;
	unsigned int pos = 0;

	if (exists(fullpath.c_str())!=0)
	{
		do
		{
			pos=fullpath.find_first_of("/", pos+1);
			pathtocreate=fullpath.substr(0, pos+1);
			//Mess.Dialog(MSG_OK,("folder: "+sourcefile+" "+int_to_string(pos)+" "+int_to_string((int)dest.size()-1)).c_str());
			if (exists(pathtocreate.c_str())!=0)
			{
				if (mkdir_one(pathtocreate)!=0) return -1;
			}
		}
		while (pos != fullpath.size()-1);
	}
	return 0;
}

string recursiveDelete(string direct)
{
	string dfile;
	DIR *dp;
	struct dirent *dirp;

	dp = opendir (direct.c_str());
	if (dp != NULL)
	{
		while ((dirp = readdir (dp)))
		{
			dfile = direct + "/" + dirp->d_name;
			if ( strcmp(dirp->d_name, ".") != 0 && strcmp(dirp->d_name, "..") != 0 && strcmp(dirp->d_name, "") != 0)
			{
				//Mess.Dialog(MSG_OK,("Testing: "+dfile).c_str());
				if (dirp->d_type == DT_DIR)
				{
					//Mess.Dialog(MSG_OK,("Is directory: "+dfile).c_str());
					recursiveDelete(dfile);
				}
				else
				{
					//Mess.Dialog(MSG_OK,("Deleting file: "+dfile).c_str());
					if ( sysFsUnlink(dfile.c_str()) != 0) return "Nao foi possivel excluir o arquivo "+dfile+"\n"+strerror(errno);
				}
			}
		}
		(void) closedir (dp);
	}
	else return "Couldn't open the directory";
	//Mess.Dialog(MSG_OK,("Deleting folder: "+direct).c_str());
	if ( rmdir(direct.c_str()) != 0) return "Nao foi possivel excluir o diretorio "+direct+"\n"+strerror(errno);
	return "";
}

string *recursiveListing(string direct)
{
	string dfile;
	DIR *dp;
	struct dirent *dirp=NULL;
	string *listed_file_names = NULL;  //Pointer for an array to hold the filenames.
	string *sub_listed_file_names = NULL;  //Pointer for an array to hold the filenames.
	int aindex=0;

	listed_file_names = new string[5000];
	sub_listed_file_names = new string[5000];
	dp = opendir (direct.c_str());
	if (dp != NULL)
	{
		while ((dirp = readdir (dp)))
		{
			if ( strcmp(dirp->d_name, ".") != 0 && strcmp(dirp->d_name, "..") != 0 && strcmp(dirp->d_name, "") != 0)
			{
				dfile = direct + "/" + dirp->d_name;
				if (dirp->d_type == DT_DIR)
				{
					sub_listed_file_names=recursiveListing(dfile);
					//Mess.Dialog(MSG_OK,("Dir: "+dfile+" "+int_to_string(sizeof(sub_listed_file_names)/sizeof(sub_listed_file_names[0]))).c_str());
					int i=0;
					while (strcmp(sub_listed_file_names[i].c_str(),"") != 0)
					{
						listed_file_names[aindex]=sub_listed_file_names[i];
						//Mess.Dialog(MSG_OK,("file: "+listed_file_names[aindex]).c_str());
						i++;
						aindex++;
					}
					//Mess.Dialog(MSG_OK,("Dir: "+dfile+" "+int_to_string(i)).c_str());
				}
				else
				{
					listed_file_names[aindex]=dfile;
					//Mess.Dialog(MSG_OK,("File: "+listed_file_names[aindex]).c_str());
					aindex++;
				}
			}
		}
		closedir(dp);
	}

	return listed_file_names;
}

int check_delete(string dpath, int what)
{
	string cpath;

	cpath=dpath;
	if (what==1 || what==2)
	{
		if (cpath.find("DEL~")!=string::npos) cpath.replace( cpath.find("DEL~"), 3, ""); else return 0;
	}
	if (what==1 || what==2) replace(cpath.begin(), cpath.end(), '~', '/');
	if (what==2) if (cpath.find("dev_flash")!=string::npos) cpath.replace( cpath.find("dev_flash"), 9, "dev_blind");

	if (cpath.find("dev_blind")!=string::npos)
	{
		if (is_dev_blind_mounted()!=0) mount_dev_blind();
		if (is_dev_blind_mounted()!=0) return -1;
	}

	if (is_dir(cpath.c_str()))
	{
		recursiveDelete(cpath.c_str());
	}
	else if (exists(cpath.c_str()))
	{
		sysLv2FsUnlink(cpath.c_str());
	}

	return -1;
}

string correct_path(string dpath, int what)
{
	string cpath;

	cpath=dpath;
	if (what==1 || what==2) if (cpath.find("PS3~")!=string::npos) cpath.replace( cpath.find("PS3~"), 4, "");
	if (what==1 || what==2) replace(cpath.begin(), cpath.end(), '~', '/');
	if (what==2) if (cpath.find("dev_flash")!=string::npos) cpath.replace( cpath.find("dev_flash"), 9, "dev_blind");

	return "/"+cpath;
}

string get_app_folder(char* path)
{
	string folder;
	char * pch;
	int mcount=0;

	pch = strtok(path,"/");
	while (pch != NULL)
	{
		if (mcount<4)
		{
			if (pch==(string)DEV_TITLEID) folder=folder+"/"+(string)APP_TITLEID;
			else folder=folder+"/"+pch;
		}
		mcount++;
		pch = strtok (NULL,"/");
	}
	return folder;
}

// Detects firmware changes, not necessary as i don't use backups 

//void check_firmware_changes(string appfolder)
//{
//	if (exists("/dev_flash/vsh/resource/explore/xmb/xmbmp.cfg")!=0 && exists_backups(appfolder)==0)
//	{
//		Mess.Dialog(MSG_OK,"The system detected a firmware change. All previous backups will be deleted.");
//		string ret=recursiveDelete(appfolder+"/backups");
//		if (ret == "") Mess.Dialog(MSG_OK,"All backups deleted!\nPress OK to continue.");
//		else Mess.Dialog(MSG_ERROR,("Problem with delete!\n\nError: "+ret).c_str());
//	}
// }

int check_terms(string appfolder)
{
	if(dont_backup) return 0;
	if (exists((appfolder+"/data/terms-accepted.cfg").c_str())!=0)//terms not yet accepted
	{
		Mess.Dialog(MSG_OK,"Este \"software\" e fornecido SEM QUALQUER GARANTIA DE QUALQUER TIPO, EXPRESSA OU IMPLÍCITA, INCLUINDO, SEM LIMITAÇAO, AS GARANTIAS DE COMERCIALIZAÇAO E ADEQUAÇAO A UM DETERMINADO FIM E NAO VIOLAÇAO. Em nenhum caso o autor ou detentores de direitos autorais serao responsáveis por qualquer reclamacao, dano ou outra responsabilidade, seja em uma acao de contrato, ato ilicito ou de outra forma, decorrente, fora ou em conexao com o \"software\" ou o uso de outros negocios no \"software\".");
		Mess.Dialog(MSG_OK,"Este \"software\" e um projeto de hobby e destina-se exclusivamente a fins educacionais e de teste. É necessário que tais acoes do usuário estejam em conformidade com a legislacao local, federal e nacional.\nO autor, parceiros e associados nao toleram pirataria e NAO assumirá nenhuma responsabilidade, legal ou implicita, por qualquer uso indevido ou por qualquer perda que possa ocorrer durante o uso do \"software\".");
		Mess.Dialog(MSG_YESNO_DYES,"Você e o unico responsável pelo cumprimento das leis aplicáveis em seu pais e deve deixar de usar este software caso suas acoes durante a operacao do \"software\" levem ou possam levar à violacao ou violacao dos direitos dos respectivos detentores de direitos autorais de conteudo. \n\nVocê aceita estes termos?");
		if (Mess.GetResponse(MSG_DIALOG_BTN_YES)==1)
		{
			create_file((appfolder+"/data/terms-accepted.cfg").c_str());
			return 0;
		}
		else return -1;
	}
	return 0;
}


string copy_file(string title, const char *dirfrom, const char *dirto, const char *filename, double filesize, double copy_currentsize, double copy_totalsize, int numfiles_current, int numfiles_total, int check_flag, int showprogress)
{
	string cfrom=(string)dirfrom+(string)filename;
	string ctoo=(string)dirto+(string)filename;
	FILE *from, *to;
	string ret="";
	double percent=copy_currentsize/copy_totalsize*100, oldpercent=percent, changepercent=0, current_copy_size=0;
	string current;
	string sfilename=(string)filename;
	string scurrent_files=int_to_string(numfiles_current);
	string stotal_files=int_to_string(numfiles_total);
	string stotal_size=convert_size(copy_totalsize, "auto");

	PF.printf((title+" '"+sfilename+"'\r\n").c_str());
	PF.printf(("- source: "+cfrom+" \r\n").c_str());
	PF.printf(("- dest: "+ctoo+" \r\n").c_str());

	if ((from = fopen(cfrom.c_str(), "rb"))==NULL) return "Nao foi possivel abrir o arquivo de codigo-fonte ("+cfrom+") para leitura!";
	if (check_flag!=1)
	{
		char* buf = (char*) calloc (1, CHUNK+1);
		size_t size;
		if ((to = fopen(ctoo.c_str(), "wb"))==NULL) return "Nao foi possivel abrir o arquivo de destino ("+ctoo+") para escrita!";
		do
		{
			//draw_copy(title, dirfrom, dirto, filename, cfrom, copy_currentsize, copy_totalsize, numfiles_current, numfiles_total, countsize);
			size = fread(buf, 1, CHUNK, from);
			if(ferror(from)) return "Erro ao ler o arquivo de origem ("+cfrom+")!";
			fwrite(buf, 1, size, to);
			if (ferror(to)) return "Erro ao gravar o arquivo de destino ("+ctoo+")!";

			if (showprogress==0)
			{
				current_copy_size=current_copy_size+(double)size;
				percent=(copy_currentsize+current_copy_size)/copy_totalsize*100;
				changepercent=percent-oldpercent;
				current="Processing "+scurrent_files+" of "+stotal_files+" files ("+convert_size(copy_currentsize+current_copy_size, "auto")+"/"+stotal_size+")";
				//PF.printf((" "+int_to_string((int)percent)+"%% "+current+" \r\n").c_str());
				//PF.printf((" change"+int_to_string((int)changepercent)+"%% real"+int_to_string((int)percent)+"%% "+current+" \r\n").c_str());
				Mess.ProgressBarDialogFlip();
				if (changepercent>1)
				{
					Mess.SingleProgressBarDialogChangeMessage(current.c_str());
					Mess.ProgressBarDialogFlip();
					Mess.SingleProgressBarDialogIncrease(changepercent);
					Mess.ProgressBarDialogFlip();
					oldpercent=percent-(changepercent-(int)changepercent);
				}
			}
		}
		while(!feof(from));
		free(buf);
	}
	else
	{
		char* buf = (char*) calloc (1, CHUNK+1);
		char* buf2 = (char*) calloc (1, CHUNK+1);
		size_t size, size2;
		if ((to = fopen(ctoo.c_str(), "rb"))==NULL) return "Nao foi possivel abrir o arquivo de destino ("+ctoo+") para leitura!";
		do
		{
			size = fread(buf, 1, CHUNK, from);
			if(ferror(from)) return "Erro ao ler o arquivo de origem ("+cfrom+")!";
			size2 = fread(buf2, 1, CHUNK, to);
			if (ferror(to)) return "Erro ao ler o arquivo de destino ("+ctoo+")!";
			
// Removed check, avoid installation issues
			
//			if (size != size2) return "Source and destination files have different sizes!";//
//			if (memcmp(buf, buf2, size)!=0) return "Source and destination files are different!";

			if (showprogress==0)
			{
				current_copy_size=current_copy_size+(double)size;
				percent=(copy_currentsize+current_copy_size)/copy_totalsize*100;
				changepercent=percent-oldpercent;
				current="Processing "+scurrent_files+" of "+stotal_files+" files ("+convert_size(copy_currentsize+current_copy_size, "auto")+"/"+stotal_size+")";
				//PF.printf((" "+int_to_string((int)percent)+"%% "+current+" \r\n").c_str());
				//PF.printf((" change"+int_to_string((int)changepercent)+"%% real"+int_to_string((int)percent)+"%% "+current+" \r\n").c_str());
				Mess.ProgressBarDialogFlip();
				if (changepercent>1)
				{
					Mess.SingleProgressBarDialogChangeMessage(current.c_str());
					Mess.ProgressBarDialogFlip();
					Mess.SingleProgressBarDialogIncrease((int)changepercent);
					Mess.ProgressBarDialogFlip();
					oldpercent=percent-(changepercent-(int)changepercent);
				}
			}
		}
		while(!feof(from) || !feof(to));
		free(buf);
		free(buf2);
	}

	if (fclose(from)==EOF) return "Nao foi possivel fechar o arquivo de origem ("+cfrom+")!";
	if (fclose(to)==EOF) return "Nao foi possivel fechar o arquivo de destino ("+ctoo+")!";

	return "";
}

string copy_prepare(string appfolder, string operation, string foldername, string fw_folder, string app)
{
	DIR *dp;
	struct dirent *dirp;
	int findex=0, mountblind=0, numfiles_total=0, numfiles_current=1, j=0, i=0, showprogress=0;
	double copy_totalsize=0, copy_currentsize=0, source_size=0, dest_size=0, freespace_size=0;
	string source_paths[100], dest_paths[100], check_paths[100];
	string check_path, sourcefile, destfile, filename, dest, source, title;
	string *files_list = NULL, *final_list_source = NULL, *final_list_dest = NULL;  //Pointer for an array to hold the filenames.
	string ret="";

	if (operation=="backup")
	{
		check_path=appfolder+"/apps/"+app+"/"+fw_folder;
		dp = opendir (check_path.c_str());
		if (dp == NULL) return "Nao foi possivel abrir o diretorio "+check_path;
		while ( (dirp = readdir(dp) ) )
		{
			if ( strcmp(dirp->d_name, ".") != 0 && strcmp(dirp->d_name, "..") != 0 && strcmp(dirp->d_name, "") != 0)
			{
				if (dirp->d_type == DT_DIR)
				{
					check_paths[findex]=check_path+"/"+dirp->d_name;
					source_paths[findex]=correct_path(dirp->d_name,2);
					dest_paths[findex]=appfolder+"/backups/"+foldername+"/"+dirp->d_name;
					if (source_paths[findex].find("dev_blind")!=string::npos) mountblind=1;
					findex++;
				}
				//check zip files
			}
		}
		closedir(dp);
		title="Fazendo backup de arquivos ...";
	}
	else if (operation=="restore")
	{
		check_path=appfolder+"/backups/"+foldername;
		dp = opendir (check_path.c_str());
		if (dp == NULL) return "Nao foi possivel abrir o diretorio "+check_path;
		while ( (dirp = readdir(dp) ) )
		{
			if ( strcmp(dirp->d_name, ".") != 0 && strcmp(dirp->d_name, "..") != 0 && strcmp(dirp->d_name, "") != 0 && dirp->d_type == DT_DIR)
			{
				source_paths[findex]=check_path + "/" + dirp->d_name;
				dest_paths[findex]=correct_path(dirp->d_name,2);
				if (dest_paths[findex].find("dev_blind")!=string::npos) mountblind=1;
				findex++;
			}
		}
		closedir(dp);
		title="Restaurando arquivos ...";
	}
	else if (operation=="install")
	{
		check_path=appfolder+"/apps/"+app+"/"+fw_folder;
		dp = opendir (check_path.c_str());
		if (dp == NULL) return "Nao foi possivel abrir o diretorio "+check_path;
		while ( (dirp = readdir(dp) ) )
		{
			if ( strcmp(dirp->d_name, ".") != 0 && strcmp(dirp->d_name, "..") != 0 && strcmp(dirp->d_name, "") != 0 && dirp->d_type == DT_DIR)
			{
				if(check_delete(dirp->d_name,2)) continue;
				source_paths[findex]=check_path + "/" + dirp->d_name;
				dest_paths[findex]=correct_path(dirp->d_name,2);
				if (dest_paths[findex].find("dev_blind")!=string::npos) mountblind=1;
				findex++;
			}
			//check zip files
		}
		closedir(dp);
		title="Copiando arquivos ...";
	}

	if (mountblind==1)
	{
		if (is_dev_blind_mounted()!=0) mount_dev_blind();
		if (is_dev_blind_mounted()!=0) return "Dev_blind nao montado!";
		if (exists("/dev_flash/vsh/resource/explore/xmb/xmbmp.cfg")!=0) create_file("/dev_blind/vsh/resource/explore/xmb/xmbmp.cfg");
	}

	//count files
	final_list_source = new string[5000];
	final_list_dest = new string[5000];
	for(j=0;j<findex;j++)
	{
		if (operation=="backup") check_path=check_paths[j];
		else check_path=source_paths[j];
		//Mess.Dialog(MSG_OK,("check_path: "+check_path).c_str());
		files_list=recursiveListing(check_path);
		i=0;
		while (strcmp(files_list[i].c_str(),"") != 0)
		{
			files_list[i].replace(files_list[i].find(check_path), check_path.size()+1, "");
			sourcefile=source_paths[j]+"/"+files_list[i];
			destfile=dest_paths[j]+"/"+files_list[i];
			//Mess.Dialog(MSG_OK,(operation+"\nsource: "+sourcefile+"\ndest:"+destfile).c_str());
			if (!(operation=="backup" && exists(sourcefile.c_str())!=0))
			{
				copy_totalsize+=get_filesize(sourcefile.c_str());
				final_list_source[numfiles_total]=sourcefile;
				final_list_dest[numfiles_total]=destfile;
				filename=final_list_source[i].substr(final_list_source[i].find_last_of("/")+1);
				source=final_list_source[i].substr(0,final_list_source[i].find_last_of("/")+1);
				dest=final_list_dest[i].substr(0,final_list_dest[i].find_last_of("/")+1);
				//Mess.Dialog(MSG_OK,(operation+"\nsource: "+final_list_source[numfiles_total]+"\ndest:"+final_list_dest[numfiles_total]).c_str());
				if (dest.find("dev_blind")!=string::npos) mountblind=1;
				numfiles_total+=1;
			}
			i++;
		}
	}

	//only show progress bar if total size bigger than 512KB
	if (copy_totalsize < 1048576/2) showprogress=-1;

	//copy files
	i=0;
	if (showprogress==0) Mess.SingleProgressBarDialog(title.c_str(), "Processando arquivos...");
	while (strcmp(final_list_source[i].c_str(),"") != 0)
	{
		sourcefile=final_list_source[i];
		destfile=final_list_dest[i];
		source_size=get_filesize(sourcefile.c_str());
		dest_size=get_filesize(destfile.c_str());
		filename=final_list_source[i].substr(final_list_source[i].find_last_of("/")+1);
		source=final_list_source[i].substr(0,final_list_source[i].find_last_of("/")+1);
		dest=final_list_dest[i].substr(0,final_list_dest[i].find_last_of("/")+1);
		freespace_size=get_free_space(dest.c_str())+dest_size;
		if (source_size >= freespace_size)
		{
			if (showprogress==0) Mess.ProgressBarDialogAbort();
			return "Nao há espaco suficiente para copiar o arquivo ("+filename+") para o caminho de destino ("+dest+").";
		}
		else
		{
			if (mkdir_full(dest)!=0)
			{
				if (showprogress==0) Mess.ProgressBarDialogAbort();
				return "Nao foi possivel criar o diretorio ("+dest+").";
			}
			ret=copy_file(title, source.c_str(), dest.c_str(), filename.c_str(),source_size, copy_currentsize, copy_totalsize, numfiles_current, numfiles_total,0,showprogress);
			if (ret != "")
			{
				if (showprogress==0) Mess.ProgressBarDialogAbort();
				return ret;
			}
		}
		copy_currentsize=copy_currentsize+source_size;
		numfiles_current++;
		i++;
	}
	if (showprogress==0) Mess.ProgressBarDialogAbort();

	//check files
	i=0;
	copy_currentsize=0;
	numfiles_current=1;
	title="Verificando arquivos ...";
	if (showprogress==0) Mess.SingleProgressBarDialog(title.c_str(), "Processando arquivos...");
	while (strcmp(final_list_source[i].c_str(),"") != 0)
	{
		sourcefile=final_list_source[i];
		destfile=final_list_dest[i];
		source_size=get_filesize(sourcefile.c_str());
		filename=final_list_source[i].substr(final_list_source[i].find_last_of("/")+1);
		source=final_list_source[i].substr(0,final_list_source[i].find_last_of("/")+1);
		dest=final_list_dest[i].substr(0,final_list_dest[i].find_last_of("/")+1);
		ret=copy_file(title, source.c_str(), dest.c_str(), filename.c_str(),source_size, copy_currentsize, copy_totalsize, numfiles_current, numfiles_total,1,showprogress);
		if (ret != "")
		{
			if (showprogress==0) Mess.ProgressBarDialogAbort();
			return ret;
		}
		copy_currentsize=copy_currentsize+source_size;
		numfiles_current++;
		i++;
	}
	if (showprogress==0) Mess.ProgressBarDialogAbort();

	return "";
}