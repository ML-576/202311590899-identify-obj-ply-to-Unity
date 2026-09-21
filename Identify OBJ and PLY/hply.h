/*

Header for PLY polygon files.

- Greg Turk, March 1994

A PLY file contains a single polygonal _object_.

An object is composed of lists of _elements_.  Typical elements are
vertices, faces, edges and materials.

Each type of element for a given object has one or more _properties_
associated with the element type.  For instance, a vertex element may
have as properties three floating-point values x,y,z and three unsigned
chars for red, green and blue.

---------------------------------------------------------------

Copyright (c) 1994 The Board of Trustees of The Leland Stanford
Junior University.  All rights reserved.   
  
Permission to use, copy, modify and distribute this software and its   
documentation for any purpose is hereby granted without fee, provided   
that the above copyright notice and this permission notice appear in   
all copies of this software and that you do not sell the software.   
  
THE SOFTWARE IS PROVIDED "AS IS" AND WITHOUT WARRANTY OF ANY KIND,   
EXPRESS, IMPLIED OR OTHERWISE, INCLUDING WITHOUT LIMITATION, ANY   
WARRANTY OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.   

*/
#ifndef __HPLY_H__
#define __HPLY_H__



#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
    
#define PLY_ASCII         1      /* ascii PLY file */
#define PLY_BINARY_BE     2      /* binary PLY file, big endian */
#define PLY_BINARY_LE     3      /* binary PLY file, little endian */
#define PLY_BINARY_NATIVE 4      /* binary PLY file, same endianness as
				    current architecture */
    
#define PLY_OKAY    0           /* ply routine worked okay */
#define PLY_ERROR  -1           /* error in ply routine */

/* scalar data types supported by PLY format */

#define PLY_START_TYPE 0
#define PLY_CHAR       1
#define PLY_SHORT      2
#define PLY_INT        3
#define PLY_UCHAR      4
#define PLY_USHORT     5
#define PLY_UINT       6
#define PLY_FLOAT      7
#define PLY_DOUBLE     8
#define PLY_END_TYPE   9

#define  PLY_SCALAR  0
#define  PLY_LIST    1
/*
#define X 0
#define Y 1
#define Z 2
*/
#define myalloc(mem_size) my_alloc((mem_size), __LINE__, __FILE__)

/* memory allocation */



#ifndef ALLOCN
#define REALLOCN(PTR,TYPE,OLD_N,NEW_N)							\
        {										\
	    if ((OLD_N) == 0)                                           		\
	    {   ALLOCN((PTR),TYPE,(NEW_N));}                            		\
	    else									\
	    {								    		\
	       (PTR) = (TYPE *)realloc((PTR),(NEW_N)*sizeof(TYPE));			\
	       if (((PTR) == NULL) && ((NEW_N) != 0))					\
	       {									\
		   fprintf(stderr, "Memory reallocation failed on line %d in %s\n", 	\
		           __LINE__, __FILE__);                             		\
		   fprintf(stderr, "  tried to reallocate %d->%d\n",       		\
			   (OLD_N), (NEW_N));                              		\
		   exit(-1);								\
	       }									\
	       if ((NEW_N)>(OLD_N))							\
		   memset((char *)(PTR)+(OLD_N)*sizeof(TYPE), 0,			\
		          ((NEW_N)-(OLD_N))*sizeof(TYPE));				\
	    }										\
	}

#define  ALLOCN(PTR,TYPE,N) 					\
	{ (PTR) = (TYPE *) calloc(((unsigned)(N)),sizeof(TYPE));\
	  if ((PTR) == NULL) {    				\
	  fprintf(stderr, "Memory allocation failed on line %d in %s\n", \
		 __LINE__, __FILE__);                           \
	  exit(-1);                                             \
	  }							\
	}


#define FREE(PTR)  { free((PTR)); (PTR) = NULL; }
#endif



class HPLY{
public:
		typedef struct PlyProperty {    /* description of a property */
		
		  char *name;                           /* property name */
		  int external_type;                    /* file's data type */
		  int internal_type;                    /* program's data type */
		  int offset;                           /* offset bytes of prop in a struct */
		
		  int is_list;                          /* 1 = list, 0 = scalar */
		  int count_external;                   /* file's count type */
		  int count_internal;                   /* program's count type */
		  int count_offset;                     /* offset byte for list count */
		
		} PlyProperty;
		
		typedef struct PlyElement {     /* description of an element */
		  char *name;                   /* element name */
		  int num;                      /* �� element face 138; num=138 */
		  int size;                     /* size of element (bytes) or -1 if variable *///HJM:��readply��size��û�б���ֵ
		  int nprops;                   /* number of properties for this element */
		  PlyProperty **props;          /* list of properties in the file */
		  char *store_prop;             /* flags: property wanted by user? */
		  int other_offset;             /* offset to un-asked-for props, or -1 if none*/
		  int other_size;               /* size of other_props structure */
		} PlyElement;
		
		typedef struct PlyOtherProp {   /* describes other properties in an element */
		  char *name;                   /* element name */
		  int size;                     /* size of other_props */
		  int nprops;                   /* number of properties in other_props */
		  PlyProperty **props;          /* list of properties in other_props */
		} PlyOtherProp;
		
		typedef struct OtherData { /* for storing other_props for an other element */
		  void *other_props;
		} OtherData;
		
		typedef struct OtherElem {     /* data for one "other" element */
		  char *elem_name;             /* names of other elements */
		  int elem_count;              /* count of instances of each element */
		  OtherData **other_data;      /* actual property data for the elements */
		  PlyOtherProp *other_props;   /* description of the property data */
		} OtherElem;
		
		typedef struct PlyOtherElems {  /* "other" elements, not interpreted by user */
		  int num_elems;                /* number of other elements */
		  OtherElem *other_list;        /* list of data for other elements */
		} PlyOtherElems;
		
		typedef struct PlyFile {        /* description of PLY file */
		  FILE *fp;                     /* file pointer */
		  int file_type;                /* ascii or binary */
		  float version;                /* version number of file */
		  int nelems;                   /* number of elements of object */
		  PlyElement **elems;           /* list of elements */
		  int num_comments;             /* number of comments */ 
		  char **comments;              /* list of comments */
		  int num_obj_info;             /* number of items of object information */
		  char **obj_info;              /* list of object info items */
		  PlyElement *which_elem;       /* which element we're currently writing */
		  PlyOtherElems *other_elems;   /* "other" elements from a PLY file */
		} PlyFile;

 typedef struct PlyEdge {
  int id;
  int vert1;
  int vert2;
  int face1;
  int face2;
  char type;
  void *other_props;       /* other properties */
} PlyEdge;


typedef struct PlyVertex {
  int id;
  double coord[3];
  unsigned char nfaces;
  int *faces;
  unsigned char nedges;
 int *edges;
  char type;
  void *other_props;       /* other properties */
  char t_joint_flag;
  /* ---- 2026新增: 支持读取PLY顶点法线与顶点颜色(常见属性), 原有字段不变 ---- */
  float normal[3];         /* nx, ny, nz (文件中存在时填充) */
  float color[3];          /* red, green, blue 归一化到0..1 */
} PlyVertex;

typedef struct PlyFace {
  int id;
  unsigned char nverts;    /* number of vertex indices in list */
  int *verts;              /* vertex index list */
  unsigned char nedges;
  int *edges;
  void *other_props;       /* other properties */
  double plane_eq[4];
  char coincident_flag;
  char sliver_flag;
  char non_triangle_flag;
  char intersect_flag;
} PlyFace;



private:
public:
/*** delcaration of routines ***/
/* returns 1 if strings are equal, 0 if not */

/* ---- 2026新增: 插件模式错误处理 ------------------------------------
   原代码在文件截断/读失败等情况下直接调用 exit() 终止进程,
   在 Unity 插件中会导致整个编辑器/游戏退出。
   现改为设置 ply_error 错误码并由调用方(ReadPly/ModelBridge)检查。
   错误码: 0=正常 1=二进制读取失败 2=ASCII意外结束(EOF)
           3=无法打开文件 4=PLY头无效 10=顶点缺少xyz 11=面缺少顶点索引 */
	static int ply_error;
	static void clear_ply_error() { ply_error = 0; }
	static int  get_ply_error()   { return ply_error; }
/* ------------------------------------------------------------------ */

/* find an element in a plyfile's list */
PlyElement *find_element(PlyFile *, char *);

/* find a property in an element's list */
PlyProperty *find_property(PlyElement *, char *, int *);

/* write to a file the word describing a PLY file data type */
void write_scalar_type (FILE *, int);

/* read a line from a file and break it up into separate words */
char **get_words(FILE *, int *, char **);
char **old_get_words(FILE *, int *);

/* write an item to a file */
void write_binary_item(FILE *, int, int, unsigned int, double, int);
void write_ascii_item(FILE *, int, unsigned int, double, int);
double old_write_ascii_item(FILE *, char *, int);

/* add information to a PLY file descriptor */
void add_element(PlyFile *, char **);
void add_property(PlyFile *, char **);
void add_comment(PlyFile *, char *);
void add_obj_info(PlyFile *, char *);

/* copy a property */
void copy_property(PlyProperty *, PlyProperty *);

/* store a value into where a pointer and a type specify */
void store_item(char *, int, int, unsigned int, double);

/* return the value of a stored item */
void get_stored_item( void *, int, int *, unsigned int *, double *);

/* return the value stored in an item, given ptr to it and its type */
double get_item_value(char *, int);

/* get binary or ascii item and store it according to ptr and type */
void get_ascii_item(char *, int, int *, unsigned int *, double *);
void get_binary_item(FILE *, int, int, int *, unsigned int *, double *);

/* get a bunch of elements from a file */
void binary_get_element(PlyFile *, char *);

/* memory allocation */
char *my_alloc(int, int, char *);

/* byte ordering */
void get_native_binary_type();
void swap_bytes(char *, int);
void setup_other_props(PlyElement *elem);
void check_types();
int get_prop_type(const char *type_name);
		void ascii_get_element(HPLY::PlyFile *plyfile, char *elem_ptr);
		char *my_alloc();
		PlyFile *ply_write(FILE *, int, char **, int);
		PlyFile *ply_open_for_writing(char *, int, char **, int, float *);
		void ply_describe_element(PlyFile *, char *, int, int, PlyProperty *);
		void ply_describe_property(PlyFile *, char *, PlyProperty *);
		void ply_element_count(PlyFile *, char *, int);
		void ply_header_complete(PlyFile *);
		void ply_put_element_setup(PlyFile *, char *);
		void ply_put_element(PlyFile *, void *);
		void ply_put_comment(PlyFile *, char *);
		void ply_put_obj_info(PlyFile *, char *);
		PlyFile *ply_open_for_reading( char *, int *, char ***, int *, float *);
		void ply_get_element_setup( PlyFile *, char *, int, PlyProperty *);
		void ply_get_property(PlyFile *, char *, PlyProperty *);
		PlyOtherProp *ply_get_other_properties(PlyFile *, char *, int);
		void ply_get_element(PlyFile *, void *);
		char **ply_get_comments(PlyFile *, int *);
		char **ply_get_obj_info(PlyFile *, int *);
		void ply_close(PlyFile *);
		void ply_get_info(PlyFile *, float *, int *);
		PlyOtherElems *ply_get_other_element (PlyFile *, char *, int);
		void ply_describe_other_elements ( PlyFile *, PlyOtherElems *);
		void ply_put_other_elements (PlyFile *);
		void ply_free_other_elements (PlyOtherElems *);
		void ply_describe_other_properties(PlyFile *, PlyOtherProp *, int);
//		PlyFile *ply_read(FILE *, int *, char ***);

		PlyFile *ply_read(FILE *, int *, char ***);
		int equal_strings(const char *, const char *);
		PlyProperty **ply_get_element_description(PlyFile *, char *, int*, int*);

//HJM����
		float smallest(int* verts, PlyFile* ply, FILE* fp_v);

		
	    void read_file(FILE* stdin_f, const char* out_v_file, const char* out_f_file);
	    void get_nverts_nfaces(const char* file, int* nvf); //ȡ�ø�����ply�ļ��Ķ������

};







#endif /* !__PLY_H__ */

