// Averaging_0.cpp
//

//===============================================//
//  使用已成長區域的總平均值當作每次判斷的基準，並   //
//  且判斷3X3範圍內的每一個pixel，是否小於或大於該  //
//  基準的正負一個閥值(每成長一個就重算avg)        //
//==============================================//

bool C3DProcess::Region_Growing(Seed_s& seed)
{
	const int Row = ROW;
	const int Col = COL;
	const int TotalSlice = Total_Slice;
	register int i, j, k;

	short S_HU = 0;
	short N_HU = 0;
	short S_pixel = 0;
	short N_pixel = 0;

	int count = 0;
	int Kernel = 3;
	int range = (Kernel - 1) / 2;

	CWait* m_wait = new CWait();
	m_wait->Create(IDD_DIALOG_WAIT);
	m_wait->ShowWindow(SW_NORMAL);
	m_wait->setDisplay("Region growing...");

	Seed_s temp;			
	Seed_s current;				// 當前seed
	queue<Seed_s> list;
	list.push(seed);

	double avg;
	double up_limit;
	double down_limit;
	double threshold = 10.0L;
	unsigned int n = 1;
	avg = m_pDoc->m_img[seed.z][(seed.y)*Row + (seed.x)];
	judge[seed.z][(seed.y) * Row + (seed.x)] = 1;

	while (!list.empty())
	{
		current = list.front();
		for (k = -range; k <= range; k++)
		{
			for (j = -range; j <= range; j++)
			{
				for (i = -range; i <= range; i++)
				{
					if ((current.x + i) < (Col - 1) && (current.x + i) >= 0 &&
						(current.y + j) < (Row - 1) && (current.y + j) >= 0 &&
						(current.z + k) < TotalSlice && (current.z + k) >= 0)
					{
						if (judge[current.z + k][(current.y + j) * Row + (current.x + i)] != 1)
						{
							N_pixel = m_pDoc->m_img[current.z + k][(current.y + j) * Row + (current.x + i)];

							up_limit = avg + threshold;
							down_limit = avg - threshold;

							if ((N_pixel <= up_limit) && (N_pixel >= down_limit))
							{
								temp.x = current.x + i;
								temp.y = current.y + j;
								temp.z = current.z + k;
								list.push(temp);

								judge[current.z + k][(current.y + j) * Row + (current.x + i)] = 1;
								getRamp(m_image0[((current.x + i) / 2) * 256 * 256 + ((current.y + j) / 2) * 256 + ((current.z + k + Mat_Offset + 1) / 2)],
									(float)N_pixel / 255.0F, 1);

								avg = (avg * n + N_pixel) / (n+1);
								n += 1;
							}
						}
					}
				}
			}
		}
		list.pop();
	}


	LoadVolume();
	Draw3DImage(true);
	m_wait->DestroyWindow();
	
	delete m_wait;
	return true;
}