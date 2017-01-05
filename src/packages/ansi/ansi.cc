
#include "base/std.h"
#include "base/package_api.h"

const char *code[]  = {"$NOR$","$BLK$","$RED$","$GRN$","$YEL$","$BLU$","$MAG$","$CYN$","$WHT$",
"$HIR$","$HIG$","$HIY$","$HIB$","$HIM$","$HIC$","$HIW$",
"$HBBLK$","$HBRED$","$HBGRN$","$HBYEL$","$HBBLU$","$HBMAG$","$HBCYN$","$HBWHT$",
"$BBLK$","$BRED$","$BGRN$","$BYEL$","$BBLU$","$BMAG$","$BCYN$","$BWHT$",
"$BOLD$","$HIREV$"};

const char *color[] = {"[m","[30m","[31m","[32m","[33m","[34m","[35m","[36m","[1;30m",
"[1;31m","[1;32m","[1;33m","[1;34m","[1;35m","[1;36m","[1;37m",
"[40;1m","[41;1m","[42;1m","[43;1m","[44;1m","[45;1m","[46;1m","[47;1m",
"[40m","[41m","[42m","[43m","[44m","[45m","[46m","[47m",
"[1m","[1;7m"};

static int combine_ansi (char ** ret, char * str, char ** ansi_table, int * flag);
static char *rally_ansi (char * str);

//
// 滤除 \0 至 \x20 之间的控制字元
// 并依照上表将特定字串转换成 ANSI 色码
//
#ifdef F_ANSI
void f_ansi () {
	int i, j;
	int clen, csize;
	char *need_transfer;
	char *str, *ret, *rethead;

	str = (char *)sp->u.string;

	need_transfer = NULL;

	while(*str)
	{
		if( (*str > 0 && *str < '\x20' && *str != '\n' && *str !='\t') || *str == '$' )
		{
			need_transfer = (char *)1;
			break;
		}
			
		str++;
	}
	
	if( need_transfer == NULL )
		return;
	
	str = (char *)sp->u.string;
	
	ret = rethead = new_string(SVALUE_STRLEN(sp)*2, "f_ansi");

	csize = sizeof(code) / sizeof(char *);

	while(*str)
	{
		clen = 0;
		if( *str >= 0 && *str < '\x20' && *str != '\n' && *str !='\t' ) 
		{
			if( *str == '\x1B' && str[1] == '[' ) 
			{
				i = 2;
				
				while( isdigit(str[i]) || str[i]==';' ) 
					i++;
				
				if( str[i] != 'm' ) 
				{
					str++;
					continue;
				}
			}
			else 
			{	
				str++;
				continue;
			}
		}
		
		if( *str == '$' )
		{
			for(i=0,j=0;i<csize;i++)
			{		
				while( str[j] && str[j] == *(code[i]+j) ) j++;
				
				if( --j && str[j] == '$' ) {
					str += j+1;
					clen = strlen(color[i]);
					
					for(j=0;j<clen;j++) *ret++ = *(color[i]+j);
					break;
				}
			}
		}

		if( clen == 0 ) 
			*ret++ = *str++;
	}
	*ret = '\0';
	
	ret = new_string(strlen(rethead), "f_ansi");
	strcpy(ret, rethead);

	FREE_MSTR(rethead);
	free_string_svalue(sp);
	put_malloced_string(ret);
}
#endif

// 
// 移除一个字串中所有的 ANSI 色码
//
#ifdef F_REMOVE_ANSI
void f_remove_ansi () {
	int i;
	char *str, *ret, *rethead;
	
	if( strchr(sp->u.string, '\x1B') == NULL )
		return;

	str = (char *)sp->u.string;

	ret = rethead = new_string(SVALUE_STRLEN(sp), "f_remove_ansi");

	while(*str) 
	{
		if( *str == '\x1B' && str[1] && str[1] == '[' ) 
		{
			i = 2;
			
			while( isdigit(str[i]) || str[i]==';' ) 
				i++;

			if( str[i] == 'm' ) {
				str += i+1;
				continue;
			}
		}
		*ret++ = *str++;
	}

	*ret = '\0';

	ret = new_string(strlen(rethead), "f_remove_ansi");
	strcpy(ret, rethead);

	FREE_MSTR(rethead);
	free_string_svalue(sp);
	put_malloced_string(ret);
}
#endif

//
// 回传一个字串不包含 ANSI 色码时的长度
//
#ifdef F_NOANSI_STRLEN
void f_noansi_strlen () {
	int i, len;
	char *str;
	
	str = (char *)sp->u.string;
	len = 0;
	while(*str) 
	{
		if( *str == '\x1B' && str[1] == '[' ) 
		{
			i = 2;
			
			while( isdigit(str[i]) || str[i]==';' )
				i++;
			
			if( str[i] == 'm' )
			{
				str += i+1;
				continue;
			}
		}
		len++; str++;
	}
	
	free_string_svalue(sp);
	put_number(len);
}
#endif

//
// 滤除重覆且不必要的 ANSI 色码并将多组色码组合起来以减低网路频宽负载
//
#ifdef F_KILL_REPEAT_ANSI
void f_kill_repeat_ansi () {
	char *str, *tmp;

	str = (char *)sp->u.string;

	tmp = rally_ansi(str);
	
	str = new_string(strlen(tmp), "f_kill_repeat_ansi");
	strcpy(str, tmp);
	
	FREE_MSTR(tmp);
	free_string_svalue(sp);
  	put_malloced_string(str);		
}
#endif

//
// 取出一个字串中的 ANSI 色码
//
#ifdef F_ANSI_PART
void f_ansi_part () {
	int i;
	char *str, *ret, *rethead;
	
	str = (char *)sp->u.string;
	
	ret = rethead = new_string(SVALUE_STRLEN(sp), "f_ansi_part");
	
	while(*str) 
	{
		if( *str == '\x1B' && str[1] == '[' ) 
		{
			i = 2;
			while( isdigit(str[i]) || str[i]==';' ) i++;

			if( str[i] == 'm' )
			{
				if( i == 2 )
				{
					str += 3; 
					continue;
				}
				i++;
				while(i--)
				{
					if( str[i] == '0' && !isdigit(str[i-1]) ) continue;
					*ret++ = *str++;
				}
			}
		}
		else str++;
	}

	*ret = '\0';
	
	ret = new_string(strlen(rethead), "f_ansi_part");
	strcpy(ret, rethead);
	
	FREE_MSTR(rethead);
	free_string_svalue(sp);
	put_malloced_string(ret);
}
#endif

//
// 将一个字串中 ANSI "背景色码" 移除
//
#ifdef F_REMOVE_BG_ANSI
void f_remove_bg_ansi () {

	char *str, *ret, *rethead;
	
	str = (char *)sp->u.string;
	
	ret = rethead = new_string(SVALUE_STRLEN(sp), "f_remove_bg_ansi");
	
	while(*str) 
	{
		if( *str == '\x1B' && str[1] == '[' ) 
		{
			*ret++ = *str++;
			*ret++ = *str++;
			
			while(*str)
			{
				if( *str == 'm' )
				{
					*ret++ = *str++;
					break;
				}
				
				if( *str == ';' )
				{
					*ret++ = *str++;
					continue;
				}
				
				if( isdigit(*str) )
				{
					if( *str == '4' )
					{
						str++;
						if( *str && isdigit(*str) )
							str++;
					}
					else 
					{
						*ret++ = *str++;
						if( *str && isdigit(*str) )
							*ret++ = *str++;
					}
				}
				else *ret++ = *str++;
			}	
		}
		else *ret++ = *str++;
	}

	*ret = '\0';
	
	str = new_string(strlen(rethead), "f_remove_gb_ansi");
	strcpy(str, rethead);
	
	FREE_MSTR(rethead);
	free_string_svalue(sp);
	put_malloced_string(str);
}
#endif

#define CLEAR_ANSI		1
#define HAVE_SAME_LEAD_ELEM 	2
#define NEW_ELEM		4
#define NEED_CLEAN		8
#define ALREADY_HAVE		16

#define ANSI_CAP	4

//
// Called by rally_ansi()
//
static int combine_ansi (char ** ret, char * str, char ** ansi_table, int * flag) {
	
	int i=0, j;
	
	while(str[i]=='\x1B')
	{
		/* 若接下来并无字元, 或字元不是 [, 则离开 combine_ansi */
		if( !str[++i] || str[i] != '[' ) break;
		
		i++;
		
		/* 凡是 ; 或 [ 或 0 一律略过 */
		while(str[i] && (str[i] == ';' || str[i] == '[' || str[i] =='0')) i++;
		
		/* 若第一个字元就是 m 的话, 则清空 ansi_table */
		if( str[i] == 'm' )
		{
			/* 清空ansi_table */
			for(j=0;j<ANSI_CAP;j++)
				*ansi_table[j] = *(ansi_table[j]+1) = 0;
			
			// mark by clode to fix back ground color bug
			//while(str[++i] && (str[i] == ' ' || str[i] == '\n'))
			while(str[++i] && str[i] == '\n' )
				*((*ret)++) = str[i];
			
			*flag |= CLEAR_ANSI;
			continue;
		}
		
		/* 对一个 ansi 内容进行 loop */
		while(str[i])
		{
			while(str[i] && str[i] == ';') i++;
			
			if( isdigit(str[i]) )
			{
				if( str[i] == '0' )
				{
					/* 清空ansi_table */
					for(j=0;j<ANSI_CAP;j++)
						*ansi_table[j] = *(ansi_table[j]+1) = 0;
				}
				
				else for(j=0;j<ANSI_CAP;j++)
				{
					/* 检查是否已有相同 ansi_elem */
					
					if( *ansi_table[j] )
					{
						if( str[i] == *ansi_table[j] )
						{
							if( str[i+1] )
							{
								if( isdigit(str[i+1]) && *(ansi_table[j]+1) )
								{
									if( *(ansi_table[j]+1) == str[i+1] )
									{
										i++;
										break;
									}
									else
									{
										*(ansi_table[j]+1) = str[++i];
										break;
									}
									
								}
								else if( !isdigit(str[i+1]) && !*(ansi_table[j]+1) ) break;
								else continue;
							}
							else return i;
						}
						else continue;
					}
					else
					{
						*ansi_table[j] = str[i];
						
						if( str[i+1] && isdigit(str[i+1]) )
							*(ansi_table[j]+1) = str[++i];
						
						break;
					}
				}
			}
			else if( str[i] == 'm' )
			{
				// mark by clode to fix back ground color bug
				//while(str[++i] && (str[i] == ' ' || str[i] == '\n')) 
				while(str[++i] && str[i] == '\n' )
					*((*ret)++) = str[i];

				if( str[i] == '\x1B' )
					break;
				else 
					return i;
			}
			else
			{
				for(j=0;j<ANSI_CAP;j++)
					*ansi_table[j] = *(ansi_table[j]+1) = 0;
				
				*((*ret)++) = '\x1B';
				return 1;
			}
			
			i++;
		}/* while */
			
	}/* while */
	
	return i;
}

//
// Called by f_kill_repeat_ansi()
//
static char *rally_ansi (char * str) {
	int i, j, k, flag;
	char *ansi_table[ANSI_CAP], *last_table[ANSI_CAP], *new_table[ANSI_CAP];
	char *ret, *rethead;
	
	ret = rethead = new_string(strlen(str)+1, "f_kill_repeat_ansi ret");
	
	for(j=0;j<ANSI_CAP;j++)
	{
		 new_table[j] = (char *)DMALLOC(2, TAG_STRING, "rally_ansi");
		last_table[j] = (char *)DMALLOC(2, TAG_STRING, "rally_ansi");
		ansi_table[j] = (char *)DMALLOC(2, TAG_STRING, "rally_ansi");
		*new_table[j] = *(new_table[j]+1) = *last_table[j] = *(last_table[j]+1) = *ansi_table[j] = *(ansi_table[j]+1) = 0;
	}
	
	while(*str)
	{
		if( *str != '\x1B' )
		{
			*ret++ = *str++;
			continue;
		}
		else
		{
			k = flag = 0;
			
			for(j=0;j<ANSI_CAP;j++)
				*new_table[j] = *(new_table[j]+1) = *ansi_table[j] = *(ansi_table[j]+1) = 0;
			
			str += combine_ansi((char **)&ret, str, ansi_table, &flag);
			
			if( flag & CLEAR_ANSI )
			{
				for(i=0;i<ANSI_CAP;i++)
				{
					if( !*last_table[i] ) break;
					
					for(j=0;j<ANSI_CAP;j++)
					{
						if( !*ansi_table[j] ) break;
						
						if( *last_table[i] == *ansi_table[j] )
						{
							flag |= HAVE_SAME_LEAD_ELEM;
							if( *(last_table[i]+1) != *(ansi_table[j]+1) )
							{
								*new_table[k] = *last_table[i] = *ansi_table[j];
								*(new_table[k++]+1) = *(last_table[i]+1) = *(ansi_table[j]+1);
							}
							break;
						}
					}
					
					if( !(flag & HAVE_SAME_LEAD_ELEM) )
						flag |= NEED_CLEAN;
					else flag ^= HAVE_SAME_LEAD_ELEM;
				}
				if( flag & NEED_CLEAN )
				{
					flag ^= NEED_CLEAN;
					
					*ret++ ='\x1B';
					*ret++ ='[';
					*ret++ ='m';
					
					for(i=0;i<ANSI_CAP;i++)
					{
						if( *ansi_table[i] )
						{
							*new_table[i] = *last_table[i] = *ansi_table[i];
							*(new_table[i]+1) = *(last_table[i]+1) = *(ansi_table[i]+1);
						}
						else
							*new_table[i] = *(new_table[i]+1) = *last_table[i] = *(last_table[i]+1) = 0;
					}
				}
				else
				{
					for(i=0;i<ANSI_CAP;i++)
					{
						if( !*ansi_table[i] ) break;
					
						for(j=0;j<ANSI_CAP;j++)
						{
							if( !*last_table[j] ) break;
							
							if( *ansi_table[i] == *last_table[j] )
								flag |= ALREADY_HAVE;
						}
						
						if( flag & ALREADY_HAVE )
							flag ^= ALREADY_HAVE;
						else for(j=0;j<ANSI_CAP;j++)
						{
								if( *last_table[j] ) continue;
								
								*new_table[k] = *last_table[j] = *ansi_table[i];
								*(new_table[k++]+1) = *(last_table[j]+1) = *(ansi_table[i]+1);
								break;
						}
					}
				}
			}
			else
			{
				k=0;
				for(i=0;i<ANSI_CAP;i++)
				{
					if( !*ansi_table[i] ) break;
					
					flag |= NEW_ELEM;
					
					for(j=0;j<ANSI_CAP;j++)
					{
						if( !*last_table[j] ) break;
						
						if( *ansi_table[i] == *last_table[j] )
						{
							flag ^= NEW_ELEM;
							
							if( *(ansi_table[i]+1) != *(last_table[j]+1) )
							{
								*new_table[k] = *ansi_table[i];
								*(new_table[k++]+1) = *(last_table[j]+1) = *(ansi_table[i]+1);
							}
							break;
						}
					}
					
					if( flag & NEW_ELEM )
					{
						flag ^= NEW_ELEM;
						
						for(j=0;j<ANSI_CAP;j++)
						{
							if( *last_table[j] ) continue;
							
							*new_table[k] = *last_table[j] = *ansi_table[i];
							*(new_table[k++]+1) = *(last_table[j]+1) = *(ansi_table[i]+1);
							break;
						}
					}
				}
			}
			
			if( *new_table[0] )
			{
				*ret++ = '\x1B';
				*ret++ = '[';
				for(j=0;j<ANSI_CAP;j++)
				{
					if( !*new_table[j] ) break;
					
					*ret++ = *new_table[j];
					
					if( *(new_table[j]+1) ) 
						*ret++ = *(new_table[j]+1);
					
					*ret++ = ';';
				}
				*(ret-1) = 'm';
			}
		}
	}
	
	*ret = '\0';
	
	for(j=0;j<ANSI_CAP;j++)
	{
		FREE(last_table[j]);
		FREE(new_table[j]);
		FREE(ansi_table[j]);
	}
		
	return rethead;
}	

