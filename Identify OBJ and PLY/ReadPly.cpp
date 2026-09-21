#include "pch.h"
#include"ReadPly.h"
//#include <iostream>
#include <vector>

#define TRUE  1
#define FALSE 0

/*
  2026修改说明(在原代码基础上做最小修改, 支持Unity插件运行):
  1. 所有 exit(12) 改为设置 HPLY::ply_error 错误码并返回, 不再终止进程。
  2. 面元素支持任意多边形: 扇形三角化(原代码 memcpy 固定3个索引,
     四边形/多边形面会越界或丢面)。三角形数超过 rm->nfaces 时自动扩容。
  3. 新增顶点法线(nx,ny,nz)与顶点颜色(red,green,blue)读取, 结果存入
     ReadPlyInCore 公共字段; 颜色按文件数据类型自动归一化。
  4. 每次读取元素后释放 other_props 内存(原代码逐元素泄漏)。
  5. 读取中途出错(文件截断)时安全中断并记录错误码。
*/

void ReadPlyInCore::read_ply_incore(FILE* stdin_f, RawMesh* rm)
{
	const char *elem_names[] = { /* list of the kinds of elements in the user's object */
  "vertex", "face", "edge"
};

	HPLY::PlyProperty vert_props[] = { /* list of property information for a vertex */
		{"x", PLY_FLOAT, PLY_DOUBLE, offsetof(HPLY::PlyVertex,coord[0]), 0, 0, 0, 0},
  {"y", PLY_FLOAT, PLY_DOUBLE, offsetof(HPLY::PlyVertex,coord[1]), 0, 0, 0, 0},
  {"z", PLY_FLOAT, PLY_DOUBLE, offsetof(HPLY::PlyVertex,coord[2]), 0, 0, 0, 0},
  {"face_indices", PLY_INT, PLY_INT, offsetof(HPLY::PlyVertex,faces),
   1, PLY_UCHAR, PLY_UCHAR, offsetof(HPLY::PlyVertex,nfaces)},
  {"edge_indices", PLY_INT, PLY_INT, offsetof(HPLY::PlyVertex,edges),
   1, PLY_UCHAR, PLY_UCHAR, offsetof(HPLY::PlyVertex,nedges)},
  /* 2026新增: 顶点法线与颜色属性 */
  {"nx", PLY_FLOAT, PLY_FLOAT, offsetof(HPLY::PlyVertex,normal[0]), 0, 0, 0, 0},
  {"ny", PLY_FLOAT, PLY_FLOAT, offsetof(HPLY::PlyVertex,normal[1]), 0, 0, 0, 0},
  {"nz", PLY_FLOAT, PLY_FLOAT, offsetof(HPLY::PlyVertex,normal[2]), 0, 0, 0, 0},
  {"red",   PLY_FLOAT, PLY_FLOAT, offsetof(HPLY::PlyVertex,color[0]), 0, 0, 0, 0},
  {"green", PLY_FLOAT, PLY_FLOAT, offsetof(HPLY::PlyVertex,color[1]), 0, 0, 0, 0},
  {"blue",  PLY_FLOAT, PLY_FLOAT, offsetof(HPLY::PlyVertex,color[2]), 0, 0, 0, 0}
};

	HPLY::PlyProperty face_props[] = { /* list of property information for a face */
		{"vertex_indices", PLY_INT, PLY_INT, offsetof(HPLY::PlyFace,verts),
   1, PLY_UCHAR, PLY_UCHAR, offsetof(HPLY::PlyFace,nverts)},
  {"edge_indices", PLY_INT, PLY_INT, offsetof(HPLY::PlyFace,edges),
   1, PLY_UCHAR, PLY_UCHAR, offsetof(HPLY::PlyFace,nedges)}
};

const HPLY::PlyProperty edge_props[] = { /* list of property information for an edge */
  {"vert1", PLY_INT, PLY_INT, offsetof(HPLY::PlyEdge,vert1),0,0,0,0},
  {"vert2", PLY_INT, PLY_INT, offsetof(HPLY::PlyEdge,vert2),0,0,0,0},
  {"face1", PLY_INT, PLY_INT, offsetof(HPLY::PlyEdge,face1),0,0,0,0},
  {"face2", PLY_INT, PLY_INT, offsetof(HPLY::PlyEdge,face2),0,0,0,0}
};


	const char *type_names[] = {
"invalid",
"char", "short", "int",
"uchar", "ushort", "uint",
"float", "double",
};

	const int ply_type_size[] = {
  0, 1, 2, 4, 1, 2, 4, 4, 8
};

	//***********************************
	int nverts,nfaces;//,nedges;
	//PlyVertex **vlist;
	//PlyFace **flist;
	//PlyEdge **edgelist;
	PlyOtherElems *other_elements = NULL;
	PlyOtherProp *vert_other,*face_other;
	int nelems;
	char **elist;
	int num_comments;
	char **comments;
	int num_obj_info;
	char **obj_info;
	static int file_type;

	unsigned char has_vedges, has_vfaces, has_fverts, has_fedges;
	//unsigned char has_vert1, has_vert2, has_face1, has_face2;
	unsigned char has_x, has_y, has_z;
	unsigned char has_nx, has_ny, has_nz;    /* 2026新增 */
	unsigned char has_red, has_green, has_blue;  /* 2026新增 */
	int color_scale_int;   /* 2026新增: 颜色为整数类型时需除以255 */
	//*********************************

	float s;
	int i,j,k;
	PlyFile *ply;
	int nprops;
	int num_elems;
	PlyProperty **plist;
	char *elem_name;
	float version;
	PlyVertex vert_temp;

	/* 2026新增: 初始化输出字段, 清除上次错误 */
	clear_ply_error();
	hasNormals = FALSE;
	hasColors  = FALSE;
	vertexCount = 0;
	triangleCount = 0;
	errorCode = 0;

/*** Read in the original PLY object ***/


//*****************************

//+++++++++++++++++++++++++++++++

	ply  = ply_read (stdin_f, &nelems, &elist);

	/* 2026新增: 头解析失败保护(原代码 ply 为 NULL 时直接崩溃) */
	if (ply == NULL)
	{
		errorCode = 4;
		return;
	}


	ply_get_info (ply, &version, &file_type);



	for (i = 0; i < nelems; i++)
	{

	  	/* get the description of the first element */
	  	elem_name = elist[i];
	  	plist = ply_get_element_description (ply, elem_name, &num_elems, &nprops);

	  	if (equal_strings ("vertex", elem_name))
		{

  	    	/* create a vertex list to hold all the vertices */
  	    	//ALLOCN(vlist, PlyVertex*, num_elems);
  	    	nverts = num_elems;
  	    	vertexCount = num_elems;

  	    	/* set up for getting vertex elements */
  	    	has_x = has_y = has_z = has_vfaces = has_vedges = FALSE;;
			has_nx = has_ny = has_nz = FALSE;            /* 2026新增 */
			has_red = has_green = has_blue = FALSE;      /* 2026新增 */
			color_scale_int = 255;                       /* 2026新增 */

  	    	for (j=0; j<nprops; j++)
  	    	{
				if (equal_strings("x", plist[j]->name))
				{
					ply_get_property (ply, elem_name, &vert_props[0]);  // x
					has_x = TRUE;
				}
				else if (equal_strings("y", plist[j]->name))
				{
					ply_get_property (ply, elem_name, &vert_props[1]);  // y
					has_y = TRUE;
				}
				else if (equal_strings("z", plist[j]->name))
				{
					ply_get_property (ply, elem_name, &vert_props[2]);  // z
					has_z = TRUE;
				}
				/* 2026修改: 原代码发现face_indices/edge_indices时 exit(12);
				   现改为不请求这些属性(作为other props跳过), 兼容性更好 */
				else if (equal_strings("nx", plist[j]->name))
				{
					ply_get_property (ply, elem_name, &vert_props[5]);  // nx
					has_nx = TRUE;
				}
				else if (equal_strings("ny", plist[j]->name))
				{
					ply_get_property (ply, elem_name, &vert_props[6]);  // ny
					has_ny = TRUE;
				}
				else if (equal_strings("nz", plist[j]->name))
				{
					ply_get_property (ply, elem_name, &vert_props[7]);  // nz
					has_nz = TRUE;
				}
				else if (equal_strings("red", plist[j]->name))
				{
					ply_get_property (ply, elem_name, &vert_props[8]);  // red
					has_red = TRUE;
					/* 颜色类型为整数时(0..255)需要归一化 */
					int t = plist[j]->external_type;
					if (t >= PLY_CHAR && t <= PLY_UINT) color_scale_int = 255;
					else color_scale_int = 1;
				}
				else if (equal_strings("green", plist[j]->name))
				{
					ply_get_property (ply, elem_name, &vert_props[9]);  // green
					has_green = TRUE;
				}
				else if (equal_strings("blue", plist[j]->name))
				{
					ply_get_property (ply, elem_name, &vert_props[10]); // blue
					has_blue = TRUE;
				}
      		}
      		vert_other = ply_get_other_properties (ply, elem_name,
								     offsetof(PlyVertex,other_props));

      		//test for necessary properties
			/* 2026修改: 原代码此处 exit(12), 改为设置错误码返回 */
			if (!((has_x) && (has_y) && (has_z)))
      		{
				fprintf(stderr, "Vertices must have x, y, and z coordinates\n");
				errorCode = 10;
				return;
      		}

			hasNormals = (has_nx && has_ny && has_nz) ? TRUE : FALSE;
			hasColors  = (has_red && has_green && has_blue) ? TRUE : FALSE;


      		// grab all the vertex elements
      		for (j = 0; j < num_elems; j++)
			{
				memset(&vert_temp, 0, sizeof(vert_temp));   /* 2026新增: 防止文件缺属性时读到未初始化数据 */
				ply_get_element(ply, (void *)&vert_temp);

				/* 2026新增: 出错(文件截断)安全中断 */
				if (get_ply_error())
				{
					if (vert_temp.other_props) free(vert_temp.other_props);
					errorCode = get_ply_error();
					return;
				}
				/* 2026新增: 释放逐元素的other_props内存(原代码泄漏) */
				if (vert_temp.other_props)
				{
					free(vert_temp.other_props);
					vert_temp.other_props = NULL;
				}

				for(k=0;k<3;k++)
				{
					s=(float)vert_temp.coord[k];
					rm->vlist[j].coord[k]=s;
					//fwrite(&s,sizeof(float),1,p_out);

					/* 2026新增: 法线直接写入RawMesh法线列表; 无文件法线时由调用方计算 */
					if (hasNormals)
						rm->nlist[j].coord[k]=vert_temp.normal[k];
				}
				/* 2026新增: 顶点颜色归一化后保存到输出字段 */
				if (hasColors)
				{
					vertexColors.push_back(vert_temp.color[0] / color_scale_int);
					vertexColors.push_back(vert_temp.color[1] / color_scale_int);
					vertexColors.push_back(vert_temp.color[2] / color_scale_int);
				}
				//fwrite(&j, sizeof(int),1,p_out);
			}
		}
    	else if (equal_strings ("face", elem_name))
    	{

      		/* create a list to hold all the face elements */
      		//ALLOCN(flist, PlyFace *, num_elems);
      		nfaces = num_elems;

      		/* set up for getting face elements */
      		has_fverts = has_fedges = FALSE;

      		for (j=0; j<nprops; j++)
      		{
			  	if (equal_strings("vertex_indices", plist[j]->name))
			  	{
			  	    ply_get_property (ply, elem_name, &face_props[0]);
			  	    has_fverts = TRUE;
			  	}
			  	/* 2026修改: 原代码发现edge_indices时 exit(12), 现改为跳过 */
      		}
      		face_other = ply_get_other_properties (ply, elem_name,
								     offsetof(PlyFace,other_props));

      		/* test for necessary properties */
			/* 2026修改: 原代码此处 exit(12), 改为设置错误码返回 */
      		if (!has_fverts)
      		{
						fprintf(stderr,"PlyFaces must have vertex indices\n");
						errorCode = 11;
						return;
      		}

			/* 2026新增: 用动态数组收集三角化结果, 支持多边形面扇形三角化 */
			std::vector<int> tris;
			tris.reserve((size_t)num_elems * 3);

      		/* grab all the face elements */
      		//fp_v=fopen(out_v_file,"rb");
			for (j = 0; j < num_elems; j++)
			{
				PlyFace temp_f;
				/* 2026新增: 初始化, 防止文件截断时使用未初始化指针 */
				memset(&temp_f, 0, sizeof(temp_f));
        		ply_get_element (ply, (void *)(&temp_f));

				if (get_ply_error())
				{
					if (temp_f.verts) free(temp_f.verts);
					if (temp_f.other_props) free(temp_f.other_props);
					errorCode = get_ply_error();
					return;
				}
				/* 2026新增: 释放逐元素的other_props内存(原代码泄漏) */
				if (temp_f.other_props)
				{
					free(temp_f.other_props);
					temp_f.other_props = NULL;
				}

				/* 2026修改: 原代码 memcpy 固定3个索引(只支持三角面);
				   现按实际顶点数扇形三角化, 支持四边形/多边形面 */
				if (temp_f.nverts >= 3 && temp_f.verts != NULL)
				{
					for (k = 2; k < (int)temp_f.nverts; k++)
					{
						tris.push_back(temp_f.verts[0]);
						tris.push_back(temp_f.verts[k-1]);
						tris.push_back(temp_f.verts[k]);
					}
				}

				free(temp_f.verts);temp_f.verts=NULL;
			}
			 // fclose(fp_f);

			/* 2026新增: 将三角化结果写回RawMesh, 超出原容量时重新分配 */
			int triCount = (int)(tris.size() / 3);
			if (triCount > rm->nfaces && triCount > 0)
			{
				delete[] rm->flist;
				rm->flist = new RawMesh::Face[triCount];
			}
			rm->nfaces = triCount;
			triangleCount = triCount;
			for (k = 0; k < triCount; k++)
			{
				rm->flist[k].v[0] = tris[k*3+0];
				rm->flist[k].v[1] = tris[k*3+1];
				rm->flist[k].v[2] = tris[k*3+2];
			}
    	}
    	else
    		other_elements = ply_get_other_element (ply, elem_name, num_elems);
			/* 2026修改: 原代码遇"edge"元素时 exit(12), 现统一作为other element跳过 */
  	}

  	comments = ply_get_comments (ply, &num_comments);
  	obj_info = ply_get_obj_info (ply, &num_obj_info);

  	//*******************************
  	//HJM: for debug

  	// for(i=0;i<nfaces;i++)
	//  {if(i%10==0)fprintf(stderr," flist[%d]->: nverts=%d\tnedges=%d\tplane_eq[0]=%d\n",i,flist[i]->nverts,flist[i]->nedges,flist[i]->plane_eq[0]);}
	/*
  	for(i=0;i<nverts;i++)
  	{
		  if(1)//i<100)
			  printf("vlist[%d]->: coord(0)=%f\tcoord(1)=%f\tcoord(2)=%f\n",i,vlist[i]->coord[0],vlist[i]->coord[1],vlist[i]->coord[2]);
		  else break;
  	}

  	for(i=0;i<nfaces;i++)
  	{
		  if(1)//i<100)
			  printf("flist[%d]->: vert(0)=%d\tvert(1)=%d\tvert(2)=%d\n",i,flist[i]->verts[0],flist[i]->verts[1],flist[i]->verts[2]);
		  else break;
  	}

	 */ //**************************************
  	ply_close (ply);

	/* 2026新增: 记录读取过程中的底层错误(如文件截断) */
	errorCode = get_ply_error();
}
