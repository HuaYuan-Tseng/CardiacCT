
// Threshold & Deviation.cpp
//

//=============================================//
//  單純用"Threshold"與"像素值的差"進行區域成長  //
//============================================//

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
					if (judge[(current.z + k)][(current.y + j)*Row + (current.x + i)] != 1)
					{
						S_pixel = m_pDoc->m_img[(current.z)][(current.y)*Row + (current.x)];
						N_pixel = m_pDoc->m_img[(current.z + k)][(current.y)*Row + (current.x + i)];
						
						if (abs(N_pixel - S_pixel) <= 5 && N_pixel >= 180)
						{
							temp.x = current.x + i;
							temp.y = current.y + j;
							temp.z = current.z + k;
							
							list.push(temp);

							judge[(current.z + k)][(current.y + j)*Row + (current.x + i)] = 1;

							getRamp(m_image0[((current.x + i) / 2) * 256 * 256 + ((current.y + j) / 2) * 256 + ((current.z + k + Mat_Offset + 1) / 2)],
								(float)N_pixel / 255.0F / 2, 1);
						}
					}
				}
			}
		}
	}
	list.pop();
}