
#ifndef _RAWMESH_H_
#define _RAWMESH_H_
#include <iostream>
#include <vector>
#include <stdlib.h>
#include <float.h>
#include<cmath>


//×°Àà»ò½Ð×öÒ»¸ö¶à±àÐ´Ò»¸ö¼òµ¥Àà,ÓÃÓÚ½âÎöplyÎÄ¼þ
class RawMesh
{
/* 2026修改: 原Vertex/Face/Edge类型定义位于class默认private区,
   ReadPly三角化扩容时无法在外部声明该类型, 移到public区;
   三个结构体定义本身未做任何改动 */
public:
	typedef struct Vertex
	{
		float coord[3];
	}	Vertex;

	typedef struct Face		// 只考虑三角形面
	{
		int v[3];		// 组成面的顶点指针
	}	Face;

	typedef struct Edge
	{
		int vert[2];		// 组成边的顶点指针, 按小到大
	}	Edge;

private:


public:
	int nverts;
	int nfaces;
	int nedges;
	float bb[6];
	float center[3];

	Vertex *vlist;
	Face   *flist;
	Edge   *elist;
    Vertex *nlist;

	RawMesh()
	{
		vlist=NULL;
		flist=NULL;
		elist=NULL;
		nlist=NULL;
	}
	void init(int num_verts, int num_faces, int num_edges = 0 )
	{
		clear_vfe();
		nverts=num_verts;
		nfaces=num_faces;
		nedges=num_edges;
		elist= NULL;

		vlist = new Vertex[num_verts];
		flist = new Face[num_faces];
		nlist =new Vertex[num_verts];
		memset(nlist, 0, num_verts*3*sizeof(float));

		if(num_edges > 0)
			elist= new Edge[num_edges];

		if( !vlist || !flist || (num_edges && !elist ) )
		{
			std::cout<<"RawMesh(): memory allocation error!" <<std::endl;
			exit(-1);
		}
	}

	~RawMesh()
	{
		clear_vfe();
	}
	void clear_vfe()	//Ê±ÎªÁË¼õÉÙÄÚ´æ,·ÀÖ¹Ôû²ÎÊýµÄ¿Õ¼äÊÍ·Å
	{
		if(vlist)
		{
			delete[] vlist;
			vlist=NULL;
		}
		if(flist)
		{
			delete[] flist;
			flist=NULL;
		}
		if(elist)
		{
			delete[] elist;
			elist=NULL;
		}
		if(nlist)
		{
			delete[] nlist;
			nlist=NULL;
		}

	}
	void bb_center()
	{
		bb[1]=bb[3]=bb[5]=FLT_MIN;
		bb[0]=bb[2]=bb[4]=FLT_MAX;
		for(int i=0;i<nverts;i++)
		{

			if(vlist[i].coord[0]<bb[0]) bb[0]=vlist[i].coord[0];
			if(vlist[i].coord[0]>bb[1]) bb[1]=vlist[i].coord[0];
			if(vlist[i].coord[1]<bb[2]) bb[2]=vlist[i].coord[1];
			if(vlist[i].coord[1]>bb[3]) bb[3]=vlist[i].coord[1];
			if(vlist[i].coord[2]<bb[4]) bb[4]=vlist[i].coord[2];
			if(vlist[i].coord[2]>bb[5]) bb[5]=vlist[i].coord[2];
		}
		center[0]=(bb[0]+bb[1])/2.0f;
		center[1]=(bb[2]+bb[3])/2.0f;
		center[2]=(bb[4]+bb[5])/2.0f;
	}

	void bb_normolize()
	{
		/* 2026新增: 防止包围盒尺寸为0时除零 */
		float span = 0.5*(bb[1]-bb[0]);
		if (span <= 0) span = 1.0f;
		for(int i=0;i<nverts;i++)
		{
			vlist[i].coord[0]=(vlist[i].coord[0]-center[0])/span;
			vlist[i].coord[1]=(vlist[i].coord[1]-center[1])/span;
			vlist[i].coord[2]=(vlist[i].coord[2]-center[2])/span;
		}
		////////////////////////////
		center[0]=0;
		center[1]=0;
		center[2]=0;

		bb[1]=1;
		bb[0]=-1;
	}

	void unit_vector(float* v, float* n)		//ÓÃn·µ»Øµ¥Î»ÏòÁ¿
	{
		/* 2026修改: 长度为0时(退化面)返回安全值, 避免产生NaN法线 */
		float lenth=sqrt(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);
		if (lenth < 1e-12f)
		{
			n[0]=0; n[1]=1; n[2]=0;
			return;
		}
		n[0]=v[0]/lenth;
		n[1]=v[1]/lenth;
		n[2]=v[2]/lenth;
	}

	void normal_for_v()
	{
		int i,j,k;
		Face f; Vertex v1,v2,v3,norm;
		float u[3],v[3];


		for(i=0;i<nfaces;i++)
		{
			f=flist[i];
			v1=vlist[f.v[0]];
			v2=vlist[f.v[1]];
			v3=vlist[f.v[2]];

			u[0]=v2.coord[0]-v1.coord[0];
			u[1]=v2.coord[1]-v1.coord[1];
			u[2]=v2.coord[2]-v1.coord[2];
			v[0]=v3.coord[0]-v1.coord[0];
			v[1]=v3.coord[1]-v1.coord[1];
			v[2]=v3.coord[2]-v1.coord[2];
			norm.coord[0]=u[1]*v[2]-u[2]*v[1];
			norm.coord[1]=u[2]*v[0]-u[0]*v[2];
			norm.coord[2]=u[0]*v[1]-u[1]*v[0];

			for (j=0;j<3;j++)
			{
				for(k=0;k<3;k++)
				{
					nlist[f.v[j]].coord[k] += (norm.coord[k]);
					//std::cout<<nlist[f.v[j]].coord[k]<<std::endl;
				}
			}

		}

		for(i=0;i<nverts;i++)
		{
			float v[3];
			for(int j=0;j<3;j++)
				v[j]=nlist[i].coord[j];
			unit_vector(v, nlist[i].coord);
		}
	}

	void DrawObject()
	{
#ifndef NO_OPENGL_RENDER   /* 2026: 无OpenGL环境(Unity插件)下不编译渲染代码, 渲染由Unity负责 */
		int i;
		int j;

		//Ò»¶¨Òª×¢Òâ£¬Èç¹ûÄ£ÐÍÃ»ÓÐnormal
		normal_for_v();

		glBegin (GL_TRIANGLES);
		for(i=0;i<nfaces;i++)
		{
			Face f=flist[i];
			for(j=0;j<3;j++)
			{
				glNormal3f (nlist[f.v[j]].coord[0],nlist[f.v[j]].coord[1],nlist[f.v[j]].coord[2]);
				glVertex3f (vlist[f.v[j]].coord[0],vlist[f.v[j]].coord[1],vlist[f.v[j]].coord[2]);
			}
		}
		glEnd ();
#endif /* NO_OPENGL_RENDER */
	}

};
#endif
