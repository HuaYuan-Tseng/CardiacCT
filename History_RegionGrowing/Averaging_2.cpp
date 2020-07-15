//
// Averaging_2.cpp
//

//===============================================//
//  使用已成長區域的總平均值當作每次判斷的基準，並   //
//  且判斷3X3範圍內的每一個pixel，是否小於或大於該  //
//  基準的正負一個閥值                            //
//  (每一次判斷的9宮格都用同一個avg，跑完一個9宮    //
//  格後，再把avg重算一次，當下個9宮格的avg)       //
//==============================================//

//===============================================//
//  與第一版不一樣的是，有再用一個queue暫存當下算出  //
//  來的avg，這樣才是利用「成長到該點時目前已成長區  //
//  域的avg」來去判斷								 //
//  (結果是有差的QQ)								 //
//===============================================//

#define _IN 
#define _OUT

typedef struct
{
	_IN		Seed_s	seed;			// 種子點
	_IN		int		kernel;			// 拜託務必保持奇數
	_IN		int		z_upLimit;		// Z軸成長上限(最多到 0)
	_IN		int		z_downLimit;	// Z軸成長下限(最多到 TotalSlice)
	_IN		double	threshold;		// 成長閾值
	_OUT	double	growingVolume;	// 成長體積
}	RG_Factor;

void C3DProcess::Region_Growing_3D(RG_Factor& factor)
{
	//	DO : 3D 區域成長
	//
	const int Row = ROW;
	const int Col = COL;
	const int TotalSlice = Total_Slice;
	const int range = (factor.kernel - 1) / 2;	// 判斷範圍
	register int i, j, k;
	unsigned int n = 1;							// 計數成長的pixel數量

	short S_pixel = 0;
	short N_pixel = 0;
	
	double avg;
	double up_limit;
	double down_limit;
	double threshold = factor.threshold;
	
	Seed_s temp;								// 當前 判斷的周圍seed
	Seed_s current;								// 當前 判斷的中心seed
	Seed_s seed = factor.seed;					// 初始seed
	queue<double> avg_que;						// 暫存某點成長判斷完，當下已成長區域的整體avg
	queue<Seed_s> sd_que;						// 暫存成長判斷為種子點的像素位置

	avg = m_pDoc->m_img[seed.z][(seed.y) * Col + (seed.x)];
	judge[seed.z][(seed.y) * Col + (seed.x)] = 1;
	avg_que.push(avg);
	sd_que.push(seed);

	while (!sd_que.empty())
	{
		avg = avg_que.front();
		current = sd_que.front();
		up_limit = avg + threshold;
		down_limit = avg - threshold;

		for (k = -range; k <= range; k++)
		{
			for (j = -range; j <= range; j++)
			{
				for (i = -range; i <= range; i++)
				{
					if ((current.x + i) < (Col) && (current.x + i) >= 0 &&
						(current.y + j) < (Row) && (current.y + j) >= 0 &&
						(current.z + k) < (factor.z_downLimit) && (current.z + k) >= factor.z_upLimit)
					{
						if (judge[current.z + k][(current.y + j) * Col + (current.x + i)] != 1)
						{
							N_pixel = m_pDoc->m_img[current.z + k][(current.y + j) * Col + (current.x + i)];

							if ((N_pixel <= up_limit) && (N_pixel >= down_limit))
							{
								temp.x = current.x + i;
								temp.y = current.y + j;
								temp.z = current.z + k;
								sd_que.push(temp);

								judge[current.z + k][(current.y + j) * Col + (current.x + i)] = 1;
								
								avg = (avg * n + N_pixel) / (n + 1);
								avg_que.push(avg);
								n += 1;
							}
						}
					}
				}
			}
		}
		avg_que.pop();
		sd_que.pop();
	}
	//TRACE1("sd : %d \n", sd_que.size());
	//TRACE1("avg : %d \n", avg_que.size());
	factor.growingVolume = (n * VoxelSpacing_X * VoxelSpacing_Y * VoxelSpacing_Z)/1000;	// 單位(cm3)
}