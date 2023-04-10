#include "filesystem.h"
#include "syscalls.h"
#include "graphics.h"
//#include "xmbmp-debug.h"
#include <io/pad.h>
#include <time.h>
#include <zlib.h>

#define MAX_OPTIONS 20

//global vars
string menu1[MAX_OPTIONS];
string menu2[MAX_OPTIONS][MAX_OPTIONS];
string menu2_path[MAX_OPTIONS][MAX_OPTIONS];
string menu3[MAX_OPTIONS];

//headers
const string currentDateTime();
int restore(string appfolder, string foldername);
int install(string appfolder, string firmware_folder, string app_choice);
int delete_all(string appfolder);
int delete_one(string appfolder, string foldername, string type);
int make_menu_to_array(string appfolder, int whatmenu, string vers, string type);
void bitmap_menu(string fw_version, string ttype, int menu_id, int msize, int selected, int choosed, int menu1_pos, int menu1_restore);

int num_options = 0;
int auto_install = 0;

s32 main(s32 argc, char* argv[]);

const string currentDateTime()
{
    time_t     now = time(0);
    struct tm  tstruct;
    char       buf[80];
    tstruct = *localtime(&now);
    strftime(buf, sizeof(buf), "%Y-%m-%d %Hh%Mm%Ss", &tstruct);

    return buf;
}

int string_array_size(string *arr)
{
	int size=0;
	while (strcmp(arr[size].c_str(),"") != 0)
	{
		size++;
	}
	return size;
}

int restore(string appfolder, string foldername)
{
	string ret="";
	string problems="\n\nEsteja ciente de que, dependendo do que voce escolher, este processo pode alterar os arquivos /dev_flash, portanto NAO DESLIGUE SEU PS3 enquanto o processo estiver em execucao.\n\nSe voce tiver alguma corrupcao apos copiar os arquivos ou o instalador fechar inesperadamente, verifique todos os arquivos antes de reiniciar e, se possivel, reinstale o firmware.";

	Mess.Dialog(MSG_YESNO_DYES,("Tem certeza de que deseja restaurar '"+foldername+"' backup?"+problems).c_str());
	if (Mess.GetResponse(MSG_DIALOG_BTN_YES)==1)
	{
		ret=copy_prepare(appfolder, "restore", foldername, "", "");
		if (ret == "") //restore success
		{
			if (is_dev_blind_mounted()==0)
			{
				unmount_dev_blind();
				Mess.Dialog(MSG_YESNO_DYES, ("O Backup'"+foldername+"' foi restaurado com sucesso.\nVocê alterou os arquivos /dev_flash, deseja reiniciar?").c_str());
				if (Mess.GetResponse(MSG_DIALOG_BTN_YES)==1) return 2;
				else return 1;
			}
			Mess.Dialog(MSG_OK,("O Backup '"+foldername+"' foi restaurado com sucesso.\nPressione OK para continuar.").c_str());
			return 1;
		}
		else //problem in the restore process so emit a warning
		{
			Mess.Dialog(MSG_ERROR,("O Backup '"+foldername+"' nao restaurou! Ocorreu um erro ao restaurar o backup!\n\nErro: "+ret+"\n\nTente restaurar novamente manualmente, se o erro persistir, seu sistema pode estar corrompido, verifique todos os arquivos e, se necessário, reinstale o firmware.").c_str());
		}
	}

	return 0;
}

int install(string appfolder, string firmware_folder, string app_choice)
{
	string ret="";
	string problems="\n\nEsteja ciente de que, dependendo do que voce escolher, este processo pode alterar os arquivos dev_flash, portanto, NAO DESLIGUE SEU PS3 enquanto o processo estiver em execucao.\n\nSe voce tiver alguma corrupcao apos copiar os arquivos ou o instalador fechar inesperadamente, verifique todos arquivos antes de reiniciar e, se possivel, reinstale o firmware.";
	//string foldername=currentDateTime()+" Antes "+app_choice;
	string foldername="Desinstalar";

	if (num_options!=1) Mess.Dialog(MSG_YESNO_DNO,("Tem certeza de que deseja instalar '"+app_choice+"'?"+problems).c_str());
	if (num_options==1 || Mess.GetResponse(MSG_DIALOG_BTN_YES)==1)
	{
		if (auto_install) ret=copy_prepare(appfolder, "backup", foldername, firmware_folder, app_choice);
		if (auto_install || ret == "") //backup success
		{
			ret=copy_prepare(appfolder, "install", "", firmware_folder, app_choice);
			if (ret == "") //copy success
			{
				if(num_options == 1)
				{
					unmount_dev_blind();
					Graphics->AppExit();
					return 1;
				}
				if (is_dev_blind_mounted()==0)
				{
					unmount_dev_blind();
					Mess.Dialog(MSG_YESNO_DYES, ("'"+app_choice+"' foi instalado com sucesso.\nVocê alterou os arquivos /dev_flash, deseja reiniciar?").c_str());
					if (Mess.GetResponse(MSG_DIALOG_BTN_YES)==1) return 2;
					else return 1;
				}
				Mess.Dialog(MSG_OK,("'"+app_choice+"' foi instalado com sucesso.\nPressione OK para continuar.").c_str());
				return 1;
			}
			else //problem in the copy process so rollback by restoring the backup
			{
				Mess.Dialog(MSG_ERROR,("'"+app_choice+"' nao foi instalado! Ocorreu um erro ao copiar os arquivos!\n\nErro: "+ret+"\n\nO backup sera restaurado.").c_str());
				return restore(appfolder, foldername);
			}
		}
		else //problem in the backup process so rollback by deleting the backup
		{
			Mess.Dialog(MSG_ERROR,("'"+app_choice+"' nao foi instalado! Ocorreu um erro ao fazer o backup dos arquivos!\n\nErro: "+ret+"\n\nBackup incompleto sera excluido.").c_str());
			if (recursiveDelete(appfolder+"/backups/"+foldername) != "") Mess.Dialog(MSG_ERROR,("Problema ao excluir o backup!\n\nErro: "+ret+"\n\nTente excluir com um gerenciador de arquivos.").c_str());
		}
	}

	return 0;
}

int delete_all(string appfolder)
{
	string ret="";
	string problems="\n\nPor favor, NAO DESLIGUE SEU PS3 enquanto o processo estiver em execucao.";

	Mess.Dialog(MSG_YESNO_DNO,("Tem certeza de que deseja excluir todos os backups?"+problems).c_str());
	if (Mess.GetResponse(MSG_DIALOG_BTN_YES)==1)
	{
		ret=recursiveDelete(appfolder+"/backups");
		if (ret == "") //delete sucess
		{
			Mess.Dialog(MSG_OK,"Todos os backups excluidos!\nPressione OK para continuar.");
			return 1;
		}
		else //problem in the delete process so emit a warning
		{
			Mess.Dialog(MSG_ERROR,("As pastas de backup nao foram excluidas! Ocorreu um erro ao excluir as pastas!\n\nErro: "+ret+"\n\nTente excluir novamente manualmente, se o erro persistir, tente outro software para excluir essas pastas.").c_str());
		}
	}
	return 0;
}

int delete_one(string appfolder, string foldername, string type)
{
	string ret="";
	string problems="\n\nPor favor, NAO DESLIGUE SEU PS3 enquanto o processo estiver em execucao.";

	Mess.Dialog(MSG_YESNO_DNO,("Tem certeza de que deseja excluir o "+type+" '"+foldername+"'?"+problems).c_str());
	if (Mess.GetResponse(MSG_DIALOG_BTN_YES)==1)
	{
		if (strcmp(type.c_str(), "backup")==0) ret=recursiveDelete(appfolder+"/backups/"+foldername);
		else if (strcmp(type.c_str(), "app")==0) ret=recursiveDelete(appfolder+"/apps/"+foldername);
		if (ret == "") //delete sucess
		{
			Mess.Dialog(MSG_OK,("The "+type+" '"+foldername+"' foi excluido!\nPressione OK para continuar.").c_str());
			return 1;
		}
		else //problem in the delete process so emit a warning
		{
			Mess.Dialog(MSG_ERROR,("Ocorreu um erro ao excluir o "+type+" '"+foldername+"'!\n\nErro: "+ret+"\n\nTente excluir novamente manualmente, se o erro persistir, tente outro software para excluir essas pastas.").c_str());
		}
	}
	return 0;
}

int make_menu_to_array(string appfolder, int whatmenu, string vers, string type)
{
	int ifw=0, iapp=0, ibackup=0;
	DIR *dp, *dp2;
	struct dirent *dirp, *dirp2;
	string direct, direct2;

	if (whatmenu==1 || whatmenu==2 || whatmenu==0)
	{
		num_options = iapp = 0;
		direct=appfolder+"/apps";
		dp = opendir (direct.c_str());
		if (dp == NULL) return -1;
		while ( (dirp = readdir(dp) ) )
		{
			if ( strcmp(dirp->d_name, ".") != 0 && strcmp(dirp->d_name, "..") != 0 && strcmp(dirp->d_name, "") != 0 && dirp->d_type == DT_DIR)
			{
				//second menu
				ifw=0;
				direct2=direct+"/"+dirp->d_name;
				dp2 = opendir (direct2.c_str());
				if (dp2 == NULL) return -1;
				while ( (dirp2 = readdir(dp2) ) )
				{
					if ( strcmp(dirp2->d_name, ".") != 0 && strcmp(dirp2->d_name, "..") != 0 && strcmp(dirp2->d_name, "") != 0 && dirp2->d_type == DT_DIR)
					{
						string fwfolder=(string)dirp2->d_name;
						string app_fwv=fwfolder.substr(0,fwfolder.find("-"));
						string app_fwt=fwfolder.substr(app_fwv.size()+1,fwfolder.rfind("-")-app_fwv.size()-1);
						string app_fwc=fwfolder.substr(app_fwv.size()+1+app_fwt.size()+1);
						if ((strcmp(app_fwv.c_str(), vers.c_str())==0 || strcmp(app_fwv.c_str(), "All")==0) && (strcmp(app_fwt.c_str(), type.c_str())==0 || strcmp(app_fwt.c_str(), "All")==0))
						{
							//Mess.Dialog(MSG_OK,(app_fwv+"-"+vers+"||"+app_fwt+"-"+type+"||"+app_fwc).c_str());
							menu2[iapp][ifw]=app_fwc;
							menu2_path[iapp][ifw]=dirp2->d_name;
							ifw++;
						}
					}
				}
				closedir(dp2);
				if (ifw>0) //has apps for the current firmware version
				{
					menu2[iapp][ifw]="Voltar ao menu principal";
					ifw++;
					menu2[iapp][ifw]="\0";
					menu1[iapp]=dirp->d_name;
					iapp++;
				}
			}
		}
		closedir(dp);
		//print(("iapp:"+int_to_string(iapp)+"\n").c_str());
		if (iapp>0)
		{
			num_options = iapp;
			menu1[iapp]="Desinstalar";
			iapp++;
			menu1[iapp]="Sair";
			iapp++;
			menu1[iapp]="\0";
		}
	}
	if(num_options != 1) auto_install = 0;
	if(auto_install) return 0;
	if (whatmenu==3 || whatmenu==0)
	{
		ibackup=0;
		direct=appfolder+"/backups";
		if (exists_backups(appfolder)==0)
		{
			num_options++;
			dp = opendir(direct.c_str());
			if (dp == NULL) return -1;
			while ( (dirp = readdir(dp) ) )
			{
				if ( strcmp(dirp->d_name, ".") != 0 && strcmp(dirp->d_name, "..") != 0 && strcmp(dirp->d_name, "") != 0 && dirp->d_type == DT_DIR)
				{
					menu3[ibackup]=dirp->d_name;
					ibackup++;
				}
			}
			closedir(dp);
			if (ibackup>0)
			{
				menu3[ibackup]="Voltar ao menu principal";
				ibackup++;
				menu3[ibackup]="\0";
			}
			else recursiveDelete(direct);
		}
	}
	return 0;
}

void bitmap_menu(string fw_version, string ttype, int menu_id, int msize, int selected, int choosed, int menu1_pos, int menu1_restore)
{
	if (auto_install) return;

	bitmap_background(fw_version, ttype);

	int j, tposy=png_logo.height+ypos(30)+ypos(30), posy=tposy+ypos(30), sizeTitleFont = ypos(40), sizeFont = ypos(30), spacing=ypos(4);
	int start_at=0, end_at=0, roll_at=0, dynamic_menu_end=0;
	string menu1_text;

	//dynamic menu calculations
	if (menu_id==1) dynamic_menu_end=msize-3;
	else if (menu_id==2) dynamic_menu_end=msize-2;
	else if (menu_id==3) dynamic_menu_end=msize-2;
	if (menu_id==1) roll_at=MENU_ROLL_OPTIONS;
	else roll_at=SUBMENU_ROLL_OPTIONS;

	if (dynamic_menu_end+1>roll_at)
	{
		if (selected<=1) start_at=0; //two first options
		else if (selected>dynamic_menu_end-roll_at+1) start_at=dynamic_menu_end-roll_at+1; //two last options
		else start_at=selected-1;
		end_at=start_at+roll_at-1;
		if (end_at>=dynamic_menu_end) end_at=dynamic_menu_end;
		if (start_at!=0) ISUp.AlphaDrawIMGtoBitmap(xpos(950),tposy+sizeFont+spacing,&png_scroll_up,&Menu_Layer);
		if (end_at!=dynamic_menu_end) ISDown.AlphaDrawIMGtoBitmap(xpos(950),tposy+roll_at*(sizeFont+spacing),&png_scroll_down,&Menu_Layer);
	}
	else end_at=dynamic_menu_end;

	if (menu_id==1)
	{
		F1.PrintfToBitmap(center_text_x(sizeTitleFont, "MENU__DE__INSTALACAO"),tposy,&Menu_Layer, 0x4b0082, sizeTitleFont, "Menu de Instalacao");
		//dynamic menu
		for(j=start_at;j<=end_at;j++)
		{
			posy=posy+sizeFont+spacing;
			F2.PrintfToBitmap(center_text_x(sizeFont, menu1[j].c_str()),posy,&Menu_Layer,menu_text_color(j, selected, choosed,-1,-1),sizeFont, "%s",menu1[j].c_str());
		}
		//static menu
		F2.PrintfToBitmap(center_text_x(sizeFont, menu1[msize-2].c_str()),ypos(550)-(2*(sizeFont+spacing)),&Menu_Layer,menu_text_color(msize-2, selected, choosed,0,menu1_restore),sizeFont, "%s",menu1[msize-2].c_str());
		F2.PrintfToBitmap(center_text_x(sizeFont, menu1[msize-1].c_str()),ypos(550),&Menu_Layer,menu_text_color(msize-1, selected, choosed,-1,-1),sizeFont, "%s",menu1[msize-1].c_str());
		//buttons
		if (selected<msize-2)
		{

			IBCross.AlphaDrawIMGtoBitmap(xpos(90),ypos(570),&png_button_cross,&Menu_Layer);
			if (string_array_size(menu2[menu1_pos])>2) F2.PrintfToBitmap(xpos(90)+png_button_cross.width+xpos(10),ypos(570)+sizeFont-ypos(5),&Menu_Layer, COLOR_WHITE, sizeFont, "Selecionar");
			else F2.PrintfToBitmap(xpos(90)+png_button_cross.width+xpos(10),ypos(570)+sizeFont-ypos(5),&Menu_Layer, COLOR_WHITE, sizeFont, "Instalar");
			IBSquare.AlphaDrawIMGtoBitmap(xpos(90),ypos(610),&png_button_square,&Menu_Layer);
			F2.PrintfToBitmap(xpos(90)+png_button_square.width+xpos(10),ypos(610)+sizeFont-ypos(5),&Menu_Layer, COLOR_WHITE, sizeFont, "Apagar");
		}
		else
		{
			IBCross.AlphaDrawIMGtoBitmap(xpos(90),ypos(610),&png_button_cross,&Menu_Layer);
			F2.PrintfToBitmap(xpos(90)+png_button_cross.width+xpos(10),ypos(610)+sizeFont-ypos(5),&Menu_Layer, COLOR_WHITE, sizeFont, "Selecionar");
		}
	}
	else if (menu_id==2)
	{
		F1.PrintfToBitmap(center_text_x(sizeTitleFont, "Install__Version"),tposy,&Menu_Layer, 0x2828ca, sizeTitleFont, "Install Version");
		//dynamic menu
		for(j=start_at;j<=end_at;j++)
		{
			posy=posy+sizeFont+spacing;
			F2.PrintfToBitmap(center_text_x(sizeFont, menu2[menu1_pos][j].c_str()),posy,&Menu_Layer,menu_text_color(j, selected, choosed,-1,-1),sizeFont, "%s",menu2[menu1_pos][j].c_str());
		}
		//static menu
		F2.PrintfToBitmap(center_text_x(sizeFont, menu2[menu1_pos][msize-1].c_str()),ypos(550),&Menu_Layer,menu_text_color(msize-1, selected, choosed,-1,-1),sizeFont, "%s",menu2[menu1_pos][msize-1].c_str());
		//buttons
		IBCross.AlphaDrawIMGtoBitmap(xpos(90),ypos(570),&png_button_cross,&Menu_Layer);
		if (selected<msize-1) F2.PrintfToBitmap(xpos(90)+png_button_cross.width+xpos(10),ypos(570)+sizeFont-ypos(5),&Menu_Layer, COLOR_WHITE, sizeFont, "Instalar");
		else F2.PrintfToBitmap(xpos(90)+png_button_cross.width+xpos(10),ypos(570)+sizeFont-ypos(5),&Menu_Layer, COLOR_WHITE, sizeFont, "Selecionar");
		IBCircle.AlphaDrawIMGtoBitmap(xpos(90),ypos(610),&png_button_circle,&Menu_Layer);
		F2.PrintfToBitmap(xpos(90)+png_button_circle.width+xpos(10),ypos(610)+sizeFont-ypos(5),&Menu_Layer, COLOR_WHITE, sizeFont, "Back");
	}
	else if (menu_id==3)
	{
		F1.PrintfToBitmap(center_text_x(sizeTitleFont, "MENU__DE__RESTAURACAO"),tposy,&Menu_Layer, 0x3d85c6, sizeTitleFont, "Menu De Restauracao");
		//dynamic menu
		for(j=start_at;j<=end_at;j++)
		{
			posy=posy+sizeFont+spacing;
			F2.PrintfToBitmap(center_text_x(sizeFont, menu3[j].c_str()),posy,&Menu_Layer,menu_text_color(j, selected, choosed,-1,-1),sizeFont, "%s",menu3[j].c_str());
		}
		//static menu
		F2.PrintfToBitmap(center_text_x(sizeFont, menu3[msize-1].c_str()),ypos(550),&Menu_Layer,menu_text_color(msize-1, selected, choosed,-1,-1),sizeFont, "%s",menu3[msize-1].c_str());
		//buttons
		if (selected<msize-1)
		{
			IBCross.AlphaDrawIMGtoBitmap(xpos(90),ypos(490),&png_button_cross,&Menu_Layer);
			F2.PrintfToBitmap(xpos(90)+png_button_cross.width+xpos(10),ypos(490)+sizeFont-ypos(5),&Menu_Layer, COLOR_WHITE, sizeFont, "Restaurar");
			IBSquare.AlphaDrawIMGtoBitmap(xpos(90),ypos(530),&png_button_square,&Menu_Layer);
			F2.PrintfToBitmap(xpos(90)+png_button_square.width+xpos(10),ypos(530)+sizeFont-ypos(5),&Menu_Layer, COLOR_WHITE, sizeFont, "Apagar");
			IBTriangle.AlphaDrawIMGtoBitmap(xpos(90),ypos(570),&png_button_triangle,&Menu_Layer);
			F2.PrintfToBitmap(xpos(90)+png_button_triangle.width+xpos(10),ypos(570)+sizeFont-ypos(5),&Menu_Layer, COLOR_WHITE, sizeFont, "Apagar todos");
		}
		else
		{
			IBCross.AlphaDrawIMGtoBitmap(xpos(90),ypos(570),&png_button_cross,&Menu_Layer);
			F2.PrintfToBitmap(xpos(90)+png_button_cross.width+xpos(10),ypos(570)+sizeFont-ypos(5),&Menu_Layer, COLOR_WHITE, sizeFont, "Selecionar");
		}
		IBCircle.AlphaDrawIMGtoBitmap(xpos(90),ypos(610),&png_button_circle,&Menu_Layer);
		F2.PrintfToBitmap(xpos(90)+png_button_circle.width+xpos(10),ypos(610)+sizeFont-ypos(5),&Menu_Layer, COLOR_WHITE, sizeFont, "Voltar");
	}
}

s32 main(s32 argc, char* argv[])
{
	padInfo2 padinfo2;
	padData paddata;
	int menu_restore=-1, menu1_position=0, menu2_position=0, menu3_position=0, mpos=0, reboot=0, temp=0, current_menu=1, msize=0, choosed=0;
	string fw_version, ttype, mainfolder, rtype="hard";
	int oldmsize=msize, oldcurrentmenu=current_menu, oldmpos=mpos;

	PF.printf("Iniciando o programa\r\n");
	PF.printf("- Inicializando os controladores\r\n");
	ioPadInit(MAX_PORT_NUM); //this will initialize the controller (7= seven controllers)
	mainfolder=get_app_folder(argv[0]);
	auto_install = check_auto(mainfolder);
	menu_restore=exists_backups(mainfolder);
	PF.printf(("- Obtendo a pasta principal do aplicativo: "+mainfolder+"\r\n").c_str());
	PF.printf("- Mostrando termos\r\n");
	if (check_terms(mainfolder)!=0) goto end;
	PF.printf("- Detectando alteracoes de firmware\r\n");
	//check_firmware_changes(mainfolder);
	fw_version=get_firmware_info("version");
	ttype=get_firmware_info("type");
	PF.printf(("- Obtendo informacoes de firmware "+fw_version+" ("+ttype+")\r\n").c_str());
	PF.printf("- Construindo menu\r\n");
	if (make_menu_to_array(mainfolder, 0,fw_version, ttype)!=0) { Mess.Dialog(MSG_ERROR,"Problema ao ler a pasta!"); goto end; }
	PF.printf("- Testing  if firmware is supported\r\n");
	if (string_array_size(menu1)==0) { Mess.Dialog(MSG_ERROR,"Sua versao de firmware nao e compativel."); goto end; }
	PF.printf("Drawing menu\r\n");
	bitmap_inititalize(int_to_string(Graphics->height)+"p",mainfolder);
	bitmap_menu(fw_version, ttype, current_menu, string_array_size(menu1), mpos, 0, menu1_position, menu_restore);
	Graphics->AppStart();
	while (Graphics->GetAppStatus())
	{
		oldmsize=msize;
		oldcurrentmenu=current_menu;
		oldmpos=mpos;
		if (current_menu==1)
		{
			msize=string_array_size(menu1);
			mpos=menu1_position;
		}
		else if (current_menu==2)
		{
			msize=string_array_size(menu2[menu1_position]);
			mpos=menu2_position;
		}
		else if (current_menu==3)
		{
			msize=string_array_size(menu3);
			mpos=menu3_position;
		}
		if (msize!=oldmsize || current_menu!=oldcurrentmenu || mpos!=oldmpos || choosed==1)
		{
			bitmap_menu(fw_version, ttype, current_menu, msize, mpos, 0, menu1_position, menu_restore);
			choosed=0;
		}
		draw_menu(choosed); usleep(1666);
		if (ioPadGetInfo2(&padinfo2)==0 || num_options==1)
		{
			for(int i=0;i<MAX_PORT_NUM;i++)
			{
				if (padinfo2.port_status[i])
				{
					ioPadGetData(i, &paddata);
					if (current_menu==1)
					{
						if (paddata.BTN_DOWN || paddata.ANA_L_V == 0x00FF || paddata.ANA_R_V == 0x00FF)
						{
							if (menu1_position<msize-1)
							{
								menu1_position++;
								if (menu1_position==msize-2 && menu_restore!=0) { menu1_position++; }
							}
							else menu1_position=0;
						}
						else if (paddata.BTN_UP || paddata.ANA_L_V == 0x0000 || paddata.ANA_R_V == 0x0000)
						{
							if (menu1_position>0)
							{
								menu1_position--;
								if (menu1_position==msize-2 && menu_restore!=0) { menu1_position--; }
							}
							else menu1_position=msize-1;
						}
						else if (paddata.BTN_CROSS || num_options==1) //Install an app
						{
							choosed=1;
							bitmap_menu(fw_version, ttype, current_menu, msize, mpos, choosed, menu1_position, menu_restore);
							draw_menu(choosed);
							if (menu1_position<msize-2)
							{
								if (menu2[menu1_position][0]=="All" && string_array_size(menu2[menu1_position])==2)
								{
									draw_menu(choosed);
									temp=install(mainfolder,menu2_path[menu1_position][0], menu1[menu1_position]);
									if (temp==2)
									{
										reboot=1;
										Graphics->AppExit();
									}
									else if (temp==1)
									{
										if (make_menu_to_array(mainfolder, 3,fw_version, ttype)!=0)
										{
											Mess.Dialog(MSG_ERROR,"Problema ao ler a pasta!");
											Graphics->AppExit();
										}
										menu_restore=exists_backups(mainfolder);
									}
									if(auto_install) goto end;
								}
								else current_menu=2;
							}
							else if (menu1_position<msize-1) current_menu=3;
							else if (menu1_position<msize) Graphics->AppExit();
						}
						else if (paddata.BTN_SQUARE) //Delete an app
						{
							if (menu1_position<msize-2)
							{
								choosed=1;
								bitmap_menu(fw_version, ttype, current_menu, msize, mpos, choosed, menu1_position, menu_restore);
								draw_menu(choosed);
								temp=delete_one(mainfolder, menu1[menu1_position], "app");
								if (temp==1)
								{
									if (make_menu_to_array(mainfolder, 1,fw_version, ttype)!=0)
									{
										Mess.Dialog(MSG_ERROR,"Problema ao ler a pasta!");
										Graphics->AppExit();
									}
								}
							}
						}
					}
					else if (current_menu==2)
					{
						if (paddata.BTN_CIRCLE) { current_menu=1; }
						else if (paddata.BTN_DOWN || paddata.ANA_L_V == 0x00FF || paddata.ANA_R_V == 0x00FF)
						{
							if (menu2_position<msize-1) { menu2_position++; }
							else menu2_position=0;
						}
						else if (paddata.BTN_UP || paddata.ANA_L_V == 0x0000 || paddata.ANA_R_V == 0x0000)
						{
							if (menu2_position>0) { menu2_position--; }
							else menu2_position=msize-1;
						}
						else if (paddata.BTN_CROSS) //Install an app
						{
							choosed=1;
							bitmap_menu(fw_version, ttype, current_menu, msize, mpos, choosed, menu1_position, menu_restore);
							draw_menu(choosed);
							if (menu2_position<msize-1)
							{
								draw_menu(choosed);
								temp=install(mainfolder, menu2_path[menu1_position][menu2_position], menu1[menu1_position]);
								if (temp==2)
								{
									reboot=1;
									Graphics->AppExit();
								}
								else if (temp==1)
								{
									if (make_menu_to_array(mainfolder, 3,fw_version, ttype)!=0)
									{
										Mess.Dialog(MSG_ERROR,"Problema ao ler a pasta!");
										Graphics->AppExit();
									}
									menu_restore=exists_backups(mainfolder);
									current_menu=1;
								}
							}
							else current_menu=1;
						}
					}
					else if (current_menu==3)
					{
						if (paddata.BTN_CIRCLE) { current_menu=1; }
						else if (paddata.BTN_DOWN || paddata.ANA_L_V == 0x00FF || paddata.ANA_R_V == 0x00FF)
						{
							if (menu3_position<msize-1) { menu3_position++; }
							else menu3_position=0;
						}
						else if (paddata.BTN_UP || paddata.ANA_L_V == 0x0000 || paddata.ANA_R_V == 0x0000)
						{
							if (menu3_position>0) { menu3_position--; }
							else menu3_position=msize-1;
						}
						else if (paddata.BTN_CROSS)
						{
							choosed=1;
							bitmap_menu(fw_version, ttype, current_menu, msize, mpos, choosed, menu1_position, menu_restore);
							draw_menu(choosed);
							if (menu3_position<msize-1) //Restore a backup
							{
								draw_menu(choosed);
								temp=restore(mainfolder, menu3[menu3_position]);
								if (temp==2)
								{
									reboot=1;
									Graphics->AppExit();
								}
							}
							else current_menu=1;
						}
						else if (paddata.BTN_SQUARE)
						{
							if (menu3_position<msize-1) //Delete a backup
							{
								choosed=1;
								bitmap_menu(fw_version, ttype, current_menu, msize, mpos, choosed, menu1_position, menu_restore);
								draw_menu(choosed);
								temp=delete_one(mainfolder, menu3[menu3_position], "backup");
								if (temp==1)
								{
									if (make_menu_to_array(mainfolder, 3,fw_version, ttype)!=0)
									{
										Mess.Dialog(MSG_ERROR,"Problema ao ler a pasta!");
										Graphics->AppExit();
									}
									menu_restore=exists_backups(mainfolder);
									if (menu_restore!=0)
									{
										current_menu=1;
										menu1_position++;
									}
								}
							}
						}
						else if (paddata.BTN_TRIANGLE)
						{
							if (menu3_position<msize-1) //Delete all backups
							{
								if (delete_all(mainfolder)==1)
								{
									make_menu_to_array(mainfolder, 3,fw_version, ttype);
									menu_restore=-1;
									current_menu=1;
								}
							}
						}
					}
				}
			}
		}
	}
	goto end;

	end:
	{
		PF.printf("Encerrando o programa\r\n");
		PF.printf("- Desinicializando gráficos\r\n");
		if (choosed==1 && reboot!=1)
		{
			bitmap_menu(fw_version, ttype, current_menu, msize, mpos, 0, menu1_position, menu_restore);
			draw_menu(0);
		}
		BMap.ClearBitmap(&Menu_Layer);
		PF.printf("- Desinicializando os controladores\r\n");
		ioPadEnd(); //this will uninitialize the controllers
		if (is_dev_blind_mounted()==0)
		{
			PF.printf("- Desmontando dev_blind\r\n");
			unmount_dev_blind();
		}
		if (reboot==1)
		{
			Mess.Dialog(MSG_YESNO_DYES, "Deseja fazer um:\n- reinicializacao HARD: ciclo de ligar/desligar (selecione Sim)\n- reinicializacao SOFT: reinicialize o kernel Lv2 (selecione Nao)");
			if (Mess.GetResponse(MSG_DIALOG_BTN_NO)==1) rtype="soft";
			PF.printf("- Excluindo arquivo de desligamento\r\n");
			sysFsUnlink((char*)"/dev_hdd0/tmp/turnoff");
			PF.printf("- Reiniciando o sistema\r\n");
			Graphics->NoRSX_Exit(); //This will uninit the NoRSX lib
			reboot_sys(rtype); //reboot
		}
		else Graphics->NoRSX_Exit(); //This will uninit the NoRSX lib
	}

	return 0;
}