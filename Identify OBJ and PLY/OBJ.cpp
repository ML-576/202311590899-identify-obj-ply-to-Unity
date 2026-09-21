// OBJ.cpp: implementation of the OBJ class.
//
//////////////////////////////////////////////////////////////////////

#include "pch.h"
#include "OBJ.h"
#include "glm.h"



#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

OBJ::OBJ()
{
	this->Obj_model = NULL;
}

OBJ::~OBJ()
{
	if(this->Obj_model != NULL)
		glmDelete(Obj_model);
}

void OBJ::readOBJ(char *filename)
{
	if(this->Obj_model!=NULL)
		glmDelete(this->Obj_model);


	this->Obj_model=glmReadOBJ(filename);
	if(this->Obj_model==NULL)   /* 2026新增: 文件打开失败时 glmReadOBJ 返回 NULL */
		return;
	glmUnitize(this->Obj_model);
	glmFacetNormals(this->Obj_model);
	glmVertexNormals(this->Obj_model, 90);


}

void OBJ::DrawOBJ()
{
#ifndef NO_OPENGL_RENDER   /* 2026: 无OpenGL环境(Unity插件)下不编译渲染代码, 渲染由Unity负责 */
	glmDraw(this->Obj_model,GLM_SMOOTH | GLM_COLOR);
#endif
}



/////////////////////////////////////////////////////////////////////////////////
























