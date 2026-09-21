#ifndef _READPLYINCORE_H_
#define _READPLYINCORE_H_

#include "hply.h"
#include "RawMesh.h"

//#pragma comment (lib, "EsortHPly.lib")

class ReadPlyInCore : public HPLY
{
	//RawMesh* rm;
public:
	void read_ply_incore(FILE* stdin_f, RawMesh* rm);
	//ReadPlyInCore(RawMesh* rm1):rm(rm1){}

	/* ---- 2026新增: 解析结果输出字段(供Unity插件桥接层查询) ----
	   原函数为void且出错即exit(), 插件模式下改为记录状态不终止进程 */
	bool  hasNormals;       // PLY文件中是否带有顶点法线(nx,ny,nz)
	bool  hasColors;        // PLY文件中是否带有顶点颜色(red,green,blue)
	int   vertexCount;      // 实际读取的顶点数
	int   triangleCount;    // 三角化后的三角形数(多边形面自动扇形三角化)
	int   errorCode;        // 0=成功, 其余见 hply.h 中错误码说明
	std::vector<float> vertexColors;  // 顶点颜色(r,g,b归一化0..1, 3*nverts)

	ReadPlyInCore()
	{
		hasNormals = false;
		hasColors  = false;
		vertexCount = 0;
		triangleCount = 0;
		errorCode = 0;
	}
};
#endif
