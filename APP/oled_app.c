#include "oled_app.h"

uint8_t oled_page = 0;
uint8_t oled_last_page = 255;  // 璁板綍涓婁竴娆￠〉闈紝鐢ㄤ簬妫�娴嬮〉闈㈠彉鍖?

/**
 * @brief Boot animation
 */
void OLED_BootAnimation(void)
{
    uint8_t i;

    OLED_Clear();

    OLED_ShowString(1, 4, "SYSTEM BOOT");
    OLED_ShowString(2, 3, "Initialize...");

    for(i = 0; i <= 100; i++)
    {
        OLED_ShowProgress(i);

        if(i < 10)
        {
            OLED_ShowNum(4, 8, i, 1);
        }
        else if(i < 100)
        {
            OLED_ShowNum(4, 8, i, 2);
        }
        else
        {
            OLED_ShowNum(4, 7, i, 3);
        }

        OLED_ShowChar(4, 8 + (i>=10?2:1) + (i==100?1:0), '%');

        HAL_Delay(20);
    }

    HAL_Delay(500);

    OLED_Clear();
    OLED_ShowString(1, 1, " HELLO CAPTAIN");
    OLED_ShowString(3, 1, "System Ready!");

    HAL_Delay(2000);
    OLED_Clear();

    oled_last_page = 255;  // 閲嶇疆锛岀‘淇濅笅娆″埛鏂?
}

void oled_init(void)
{
    OLED_Init();
}

/**
 * @brief 鐢ㄧ┖鏍煎～鍏呮暣琛?
 */
static void OLED_FillLine(uint8_t line)
{
    for(uint8_t col = 0; col < 16; col++)
    {
        OLED_ShowChar(line, col, ' ');
    }
}

/**
 * @brief 鍒锋柊椤甸潰锛堝彧濉厖涓嶆竻灞忥級
 */
static void OLED_RefreshPage(void)
{
    // 椤甸潰鍙樺寲鏃跺～鍏呮墍鏈夎涓虹┖鏍?
    if(oled_page != oled_last_page)
    {
        OLED_FillLine(1);
        OLED_FillLine(2);
        OLED_FillLine(3);
        OLED_FillLine(4);
        oled_last_page = oled_page;
    }
}

void oled_task(void)
{
    char buffer[20];

    // 椤甸潰鍙樺寲鏃跺～鍏呯┖鏍兼竻闄ゆ棫鍐呭
    OLED_RefreshPage();

    switch(oled_page)
    {
        case 0:
            OLED_ShowString(1, 1, " HELLO CAPTAIN");
            OLED_ShowString(3, 1, "System Ready!");
            break;

        case 1:
            sprintf(buffer,"温度:%.1f ℃", temp);
            OLED_ShowStr(0, 0, buffer);

            sprintf(buffer,"湿度:%.1f %%",humi);
            OLED_ShowStr(0, 4, buffer);
            break;

        case 2:
            sprintf(buffer,"烟雾:%.2f %%",ppm);
            OLED_ShowStr(0, 4, buffer);
            break;

        case 3:
            // 蹇冪巼琛�姘ф樉绀?
            if(dis_hr > 0)
            {
                sprintf(buffer,"心率  :%d bpm  ", dis_hr);
                OLED_ShowStr(0, 0, buffer);

                sprintf(buffer,"血氧:%d %%   ", dis_spo2);
                OLED_ShowStr(0, 2, buffer);

                OLED_ShowStr(0, 4, "Status: OK    ");
            }
            else
            {
                sprintf(buffer,"心率:--- bpm  ");
                OLED_ShowStr(0, 0, buffer);

                sprintf(buffer,"血氧:--- %%   ");
                OLED_ShowStr(0, 2, buffer);

                OLED_ShowStr(0, 6, "请按下    ");
            }
            break;

        case 4:
            if(Save_Data.isParseData)
            {
                sprintf(buffer,"时间:%c%c:%c%c:%c%c",
                    Save_Data.UTCTime[0], Save_Data.UTCTime[1],
                    Save_Data.UTCTime[2], Save_Data.UTCTime[3],
                    Save_Data.UTCTime[4], Save_Data.UTCTime[5]);
                OLED_ShowStr(0, 0, buffer);
            }
            else
            {
                OLED_ShowStr(0, 0, "GPS locating...");
                OLED_ShowStr(0, 4, "请移至户外使用");
            }
            break;

        case 5:
            if(Save_Data.isParseData)
            {
                sprintf(buffer,"纬度:%.4f%c", latitude, Save_Data.N_S[0]);
                OLED_ShowStr(0, 0, buffer);

                sprintf(buffer,"经度:%.4f%c", longitude, Save_Data.E_W[0]);
                OLED_ShowStr(0, 4, buffer);
            }
            else
            {
                OLED_ShowStr(0, 0, "No GPS signal");
                OLED_ShowStr(0, 4, "请移至户外使用");
            }
            break;

        case 6:
            sprintf(buffer,"Pitch:%5.1f", pitch);
            OLED_ShowStr(0, 0, buffer);

            sprintf(buffer,"Roll :%5.1f", roll);
            OLED_ShowStr(0, 2, buffer);

            sprintf(buffer,"Yaw  :%5.1f", yaw);
            OLED_ShowStr(0, 4, buffer);
            break;
		case 7:
				if(rain_flag)
				{
					sprintf(buffer,"rain_value:%d", rain_vlaue);
					OLED_ShowStr(0, 0, buffer);
					
					OLED_ShowStr(0, 2, "raining");
				}
				else
				{
					sprintf(buffer,"rain_value:%d", rain_vlaue);
					OLED_ShowStr(0, 0, buffer);
					OLED_ShowStr(0, 2, "NO raining");
				}
            break;
    }
}
