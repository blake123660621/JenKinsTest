#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <slog.h>
#include <hal.h>
#include <base.h>
#include <math.h>

#include "3a.h"



#include "3a_lite_algo.h"
#include "3a_lite_func.h"
#include "3a_platform_api.h"
#include "3a_platform_interface.h"
#include "3a_adj_params.h"
//#include "lens.h"



hal_image_color_prop_t img_prop = {0};



extern int hal_get_lens_type();
extern int hal_get_board_sensor_type();
extern int hal_get_image_hdr_level();



AAA_TAMPERING TampStatus = {
	150,	//alarmCnt
	{//OverExpo;
		.en = 0,
		.preStat = 0,
		.nowStat = 0,
		.thres1[0] = 2.0, // HDR OFF
		.thres2[0] = 0.0,
		.thres1[1] = 2.0, // HDR 2X
		.thres2[1] = 0.0,
		.thres1[2] = 2.0, // HDR 3X
		.thres2[2] = 0.0,
		.cnt = 0,
	},
	{//UnderExpo;
		.en = 0,
		.preStat = 0,
		.nowStat = 0,
		.thres1[0] = -2.0, // HDR OFF
		.thres2[0] = 0.0,
		.thres1[1] = -1.5, // HDR 2X
		.thres2[1] = 0.0,
		.thres1[2] = -1.5, // HDR 3X
		.thres2[2] = 0.0,
		.cnt = 0,
	},
	{//OutFocus;
		.en = 0,
		.preStat = 0,
		.nowStat = 0,
		.thres1[0] = 20.0, // HDR OFF
		.thres2[0] = 0.0,
		.thres1[1] = 20.0, // HDR 2X
		.thres2[1] = 0.0,
		.thres1[2] = 20.0, // HDR 3X
		.thres2[2] = 0.0,
		.cnt = 0,
	},
	{//ColorDeviation;
		.en = 0,
		.preStat = 0,
		.nowStat = 0,
		.thres1[0] = 2.0, // HDR OFF
		.thres2[0] = 0.5,
		.thres1[1] = 2.0, // HDR 2X
		.thres2[1] = 0.5,
		.thres1[2] = 2.0, // HDR 3X
		.thres2[2] = 0.5,
		.cnt = 0,
	},
};

AAA_SCENE_STATE SceneStatus = {
	.Mode = AAA_SCENE_DAY,
	.IR = 0,
};



static int aaa_isp_flag = 0;
pthread_mutex_t aaa_isp_mutex;
pthread_cond_t aaa_isp_cond;
int sensorType = HAL_IMG_SENSOR_NONE;
int sensorMode = 0;
int fpsMode = 0;
int lensType = 0;
int hdrType = 0;
int aaa_isp_init = 0;

void init3aDbgThread(void);

void aaa_isp_loadSysDebugMode(void)
{
    int val = 0;
    FILE *pfile = NULL;

    if (!access("/mnt/factory/.prop/aaaDebug1.txt",F_OK)) {
		init3aDbgThread();
        pfile = fopen("/mnt/factory/.prop/aaaDebug.txt", "r+");
        if (pfile) {
            fscanf(pfile, "%d", &val);
            fclose(pfile);
            if (val >= 0) { 
                aaa_debug_mode = val;
					slog.warn("[aaaDebug-val] =%d\n",val);
                if (aaa_debug_mode) system("/usr/bin/telnetd -l /usr/bin/login");
            }
        }
		
    }
	aaa_debug_mode= 2;
    return;
}

int aaa_isp_set_dbg_mode(void*ctx)
{
    int val = *((int*)ctx);
    
    aaa_debug_mode = val;

    return 0;
}

int aaa_isp_getLensType(void)
{
	return lensType;
}

void aaa_isp_setLensType(int type)
{
    lensType = type;
}

int aaa_isp_getSensorType(void)
{
	return sensorMode;
}

void aaa_isp_setSensorType(int type)
{
    sensorType = type;
}

int aaa_isp_getSensorMode(void)
{
    return sensorMode;
}


void aaa_isp_setSensorMode(int mode)
{
	sensorMode = mode;
}


int aaa_isp_getHDRMode(void)
{
	return hdrType;
}

void aaa_isp_setHDRMode(int mode)
{
    hdrType = mode;
}

int aaa_isp_getFpsMode(void)
{
    return fpsMode;
}


void aaa_isp_setFpsMode(int mode)
{
	fpsMode = mode;
}

void aaa_isp_setFlag(int flag, int val)
{
	if(val)
		aaa_isp_flag |= flag;
	else
		aaa_isp_flag &= ~flag;
}

int aaa_isp_getFlag(int flag)
{
	return (aaa_isp_flag & flag);
}

void aaa_isp_sendEvent(void)
{
	pthread_cond_signal(&aaa_isp_cond);
}

void aaa_isp_sendEventFlag(int flag)
{
	aaa_isp_setFlag(flag, 1);
	aaa_isp_sendEvent();
}

int aaa_isp_getInitDone(void)
{
	return aaa_isp_init;
}

long long current_timestamp(void)
{
    struct timeval te;
    gettimeofday(&te, NULL); // get current time
    long long milliseconds = te.tv_sec*1000LL + te.tv_usec/1000; // caculate milliseconds
    // printf("milliseconds: %lld\n", milliseconds);
    return milliseconds;
}

void aaa_tampering_setParam(hal_tampering_prop_t *param)
{
	TampStatus.alarmCnt = param->duration * 30;
	TampStatus.OverExpo.en = param->enable_overlight;
	TampStatus.UnderExpo.en = param->enable_overdark;
	TampStatus.OutFocus.en = param->enable_out_of_focus;
	TampStatus.ColorDeviation.en = param->enable_color_cast;
}

void aaa_scene_getParam(AAA_SCENE_STATE *Param)
{
	memcpy(Param, &SceneStatus, sizeof(AAA_SCENE_STATE));
}

void aaa_scene_setParam(AAA_SCENE_STATE Param)
{
	memcpy(&SceneStatus, &Param, sizeof(AAA_SCENE_STATE));
}

int aaa_scene_getMode(void)
{
	return SceneStatus.Mode;
}

int aaa_scene_isDay(void)
{
    return (SceneStatus.Mode == AAA_SCENE_DAY)? 1 : 0; 
}

void aaa_main_setUiParam(img_config_info_t* img_config)
{
	//******* AE *******//
    int shtIdx, agcIdx;
    int i = 0, max_idx = 0, min_idx = 0;
	int fpsMode = aaa_isp_getFpsMode();
    //int manualAgcIdx = (int)(log2(img_prop.gain_ctrl_gain)*(double)EV_STEP);
    int manualAgcIdx = img_prop.gain_ctrl_gain;
	int manualShtIdx = img_prop.expo_time;

	// Shutter Index & Time Value is Opposite.
	min_idx = AeParams->sht_idx_min;
	max_idx = AeParams->sht_idx_max;

	#if 0
    for(i = SHT_IDX_MIN; i < SHT_IDX_MAX + 1; i++){
        if((int)(AeParams->sht_tbl[i].Time * 100) >= (img_prop.expo_time * 100)) {manualShtIdx = i; break;}
    }
    if(manualShtIdx == 0) manualShtIdx = SHT_IDX_MAX;

	//manual setting upper and lower
    for(i = SHT_IDX_MIN; i < SHT_IDX_MAX + 1; i++){
        if((int)(AeParams->sht_tbl[i].Time * 100) >= (img_prop.expo_time_max * 100)) {min_idx = i; break;}
    }

    for(i = SHT_IDX_MIN; i < SHT_IDX_MAX + 1; i++){
        if((int)(AeParams->sht_tbl[i].Time * 100) >= (img_prop.expo_time_min * 100)) {max_idx = i; break;}
    }
	if(max_idx == 0) max_idx = SHT_IDX_MAX;
	#endif

    aaa_ae_Enable(0);
	aaa_ae_set_manual_sht_agc_idx(manualShtIdx, manualAgcIdx);

    if(aaa_iav_isHdrStatus() && !AE_DBG_HDR_UI){ // HDR Mode , Default
        // Anti-Flicker
        aaa_ae_set_Flicker(AE_FLICKER_OFF);

		// HDR Shutter & AGC Boundary Setting in Algo .
		//if(IRIS_ENABLE){
		//	aaa_ae_setIrisRange();
		//}

		// AE Exposure Mode
		if(img_prop.expo_mode == 3){
			aaa_ae_setExpMode(AE_EXP_IRIS); // IRIS priority
			//aaa_ae_set_irisIdx(lens_Iris_Duty_to_Pos(img_prop.irisDuty), 1);
		}
		else{
			// special case for V728 / V838 @ HDR , forcec to set max iris , avoid AE converge not smooth when iris status .
			if(aaa_isp_getSensorType() == HAL_IMG_SENSOR_OS05A20_MIPI && (aaa_isp_getLensType() == RICOM_HD027135PB_G2001 || aaa_isp_getLensType() == RICOM_HD027135PB_PT5111)){
				aaa_ae_setExpMode(AE_EXP_IRIS); // IRIS priority
				//aaa_ae_set_irisIdx(lens_Iris_Duty_to_Pos(100), 1);
			}
			else
				aaa_ae_setExpMode(AE_EXP_AUTO); // AUTO
		}

        // EV Bias
        aaa_ae_set_evBiasx10(0);

        // Dynamic AE Target
        aaa_ae_setDynAETarget(0);

        // AE Converge Speed
        aaa_ae_setSpeed(7);

		// AE WinMode Set
		aaa_ae_setWinMode(img_prop.iAEWeightingMode, img_prop.iAEWeightingX, img_prop.iAEWeightingY);

    }
    else {
        // Anti-Flicker
        // AE Shutter Time Upper / Lower & Idoor / Outdoor
        if(img_prop.flickerless_mode){
            // img_prop.flickerless_mode = 0 : off
            // img_prop.flickerless_mode = 1 : on
            aaa_ae_set_Flicker(img_prop.ACHz == 60? AE_FLICKER_60HZ : AE_FLICKER_50HZ);
            //aaa_ae_set_isInDoor_ShtRange(TRUE, max_idx, min_idx);
        }
        else{
            aaa_ae_set_Flicker(AE_FLICKER_OFF);
            aaa_ae_setShtRange(max_idx, min_idx);
        }
		//if(IRIS_ENABLE){
		//	aaa_ae_setIrisRange();
		//}

        int max_gain = (int)(log2(img_prop.gain_ctrl_gain_max)*(double)EV_STEP);
        if (max_gain > AeParams->agc_idx_max) max_gain = AeParams->agc_idx_max;
        // AE Gain Upper / Lower
        aaa_ae_setAgcRange(max_gain, (int)(log2(img_prop.gain_ctrl_gain_min)*(double)EV_STEP));

		// AE Shutter Upper / Lower
		if(aaa_iav_isHdrStatus()){ // For Debug
			aaa_ae_setShtRange(SHT0_IDX_MAX_HDR, SHT0_IDX_MIN_HDR);
		}
		else{
            aaa_ae_get_sht_agc_idx(&shtIdx, &agcIdx);
		}

        // AE Exposure Mode
        #if 1 // Full Function.
        if(img_prop.expo_mode == 0 && img_prop.gain_ctrl_mode == 0){
            aaa_ae_setExpMode(AE_EXP_AUTO); // AUTO
        }
        else if(img_prop.expo_mode == 0 && img_prop.gain_ctrl_mode == 1){
            aaa_ae_setExpMode(AE_EXP_ISO); // ISO
        }
        else if(img_prop.expo_mode == 1 && img_prop.gain_ctrl_mode == 0){
            aaa_ae_setExpMode(AE_EXP_SHUTTER); // SHUTTER
        }
        else if(img_prop.expo_mode == 1 && img_prop.gain_ctrl_mode == 1){
            aaa_ae_setExpMode(AE_EXP_MANUAL); // MANUAL
            aaa_ae_set_sht_agc_idx(manualShtIdx, manualAgcIdx, 1);
        }
        #elif 1

        if(img_prop.expo_mode == 0){
            aaa_ae_setExpMode(AE_EXP_AUTO); // AUTO
        }
        else if(img_prop.expo_mode == 1){
            aaa_ae_setExpMode(AE_EXP_MANUAL); // MANUAL
            aaa_ae_set_sht_agc_idx(idx, gainIdx, 1);
        }
        else if(img_prop.expo_mode == 2){
            aaa_ae_setExpMode(AE_EXP_LOCK); // Disable AE
        }
        else if(img_prop.expo_mode == 3){
            aaa_ae_setExpMode(AE_EXP_IRIS); // IRIS priority
            //aaa_ae_set_irisIdx(lens_Iris_Duty_to_Pos(img_prop.irisDuty), 1);
        }
        #else // For Test
        aaa_ae_setExpMode(AE_EXP_SHUTTER); // SHUTTER
        aaa_ae_set_sht_agc_idx(SHT_IDX_1_256+EV_STEP*2, agcIdx, 1);
        #endif

        // EV Bias
        aaa_ae_set_evBiasx10((int)(img_prop.expo_level*10));

        // Dynamic AE Target
        aaa_ae_setDynAETarget(img_prop.dyna_expo_target);

        // AE Converge Speed
        if(fpsMode == FPS_LOW){
			aaa_ae_setSpeed(10);
		}else{
			aaa_ae_setSpeed(img_prop.expo_converage_speed);
		}

		// AE WinMode Set
		aaa_ae_setWinMode(img_prop.iAEWeightingMode, img_prop.iAEWeightingX, img_prop.iAEWeightingY);

    }

    // Smart IR
    aaa_ae_SmartIR_Enable(img_prop.smart_ir_enable);

	// BLC
	//aaa_ae_BLC_Enable(img_prop.blc_enable);
	//aaa_ae_setBLCLevel(img_prop.blc_level);
	if(img_prop.blc_level == -2)         aaa_ae_setBLCSlope(BLC_LEVEL_DARKER_2);
	else if(img_prop.blc_level == -1)    aaa_ae_setBLCSlope(BLC_LEVEL_DARKER_1);
	else if(img_prop.blc_level == 0)     aaa_ae_setBLCSlope(BLC_LEVEL_NORMAL);
	else if(img_prop.blc_level == 1)     aaa_ae_setBLCSlope(BLC_LEVEL_LIGHTER_1);
	else if(img_prop.blc_level == 2)     aaa_ae_setBLCSlope(BLC_LEVEL_LIGHTER_2);
	else if(img_prop.blc_level == 3)     aaa_ae_setBLCSlope(BLC_LEVEL_LIGHTER_2);

	aaa_ae_setReConvergeFlag(1);//aaa_ae_reConverge();

    aaa_ae_Enable(1);

    //******* AWB *******//
    // WB Auto / Manual
	switch(img_prop.wb_mode){
		case HAL_MWB_MODE_AUTO:
			aaa_awb_Enable(1);
			switch(img_prop.wb_converage_range){
				case 0: aaa_awb_setLimit(WB_8000K_R+WB_RANGE_UPPER, WB_2800K_R-WB_RANGE_LOWER, WB_2800K_B+WB_RANGE_UPPER, WB_8000K_B-WB_RANGE_LOWER);break;
				case 1: aaa_awb_setLimit(WB_10000K_R+WB_RANGE_UPPER, WB_2400K_R-WB_RANGE_LOWER, WB_2400K_B+WB_RANGE_UPPER, WB_10000K_B-WB_RANGE_LOWER);break;
				case 2: aaa_awb_setLimit(WB_12000K_R+WB_RANGE_UPPER, WB_2000K_R-WB_RANGE_LOWER, WB_2000K_B+WB_RANGE_UPPER, WB_12000K_B-WB_RANGE_LOWER);break;
			}
			break;

		case HAL_MWB_MODE_INDOOR:
			aaa_awb_Enable(1);
			aaa_awb_setLimit(WB_8000K_R+WB_RANGE_UPPER, WB_2000K_R-WB_RANGE_LOWER, WB_2000K_B+WB_RANGE_UPPER, WB_8000K_B-WB_RANGE_LOWER);
			break;

		case HAL_MWB_MODE_OUTDOOR:
			aaa_awb_Enable(0);
			aaa_awb_setCustomWBGain(AwbParams->wb_TSUBOSAKA_r, WB_GAIN_BASE, AwbParams->wb_TSUBOSAKA_b);
			//aaa_awb_Enable(1);
			//aaa_awb_setLimit(WB_12000K_R+WB_RANGE_UPPER, WB_2800K_R-WB_RANGE_LOWER, WB_2800K_B+WB_RANGE_UPPER, WB_12000K_B-WB_RANGE_LOWER);
			break;



		case HAL_MWB_MODE_MANUAL:
			aaa_awb_Enable(0);
			aaa_awb_setCustomWBGain(img_prop.wb_rgain<<2, 4096, img_prop.wb_bgain<<2);
			break;

		case HAL_MWB_MODE_CUSTOM:
			aaa_awb_Enable(0);

			unsigned short rGain, gGain, bGain;
			aaa_awb_getCurrentGain(&rGain, &gGain, &bGain);
			aaa_awb_setCustomWBGain(rGain, gGain, bGain);
			break;

		case HAL_MWB_MODE_FLUORESCENT:
			aaa_awb_Enable(0);
			aaa_awb_setCustomWBGain(WB_D65_R, WB_GAIN_BASE, WB_D65_B);
			break;
	}

    // WB Converge Speed
    if(fpsMode == FPS_LOW){
		aaa_awb_setSpeed(10);
	}
	else{
	    aaa_awb_setSpeed(img_prop.wb_converage_speed);
	}
	
    // WB Converge Shift
    aaa_awb_setConvergeShift(img_prop.wb_converage_rshift, img_prop.wb_converage_bshift);


	AAA_SCENE_STATE scene;
	aaa_scene_getParam(&scene);
	int isDay = img_prop.at_day_mode? AAA_SCENE_DAY : AAA_SCENE_NIGHT;
	if(isDay != scene.Mode){
		scene.Mode = isDay;
		aaa_scene_setParam(scene);

		aaa_platform_adj_init();

		//extern void enable_cc_again(void);
		//enable_cc_again();
	}


	if (!aaa_adj_isInit()) {
		aaa_adj_set_brightness_value(&img_prop.brightness);
		aaa_adj_set_saturation_value(&img_prop.saturation);
		aaa_adj_set_hue_value(&img_prop.hue);
		aaa_adj_set_bw_value(!img_prop.at_day_mode && !img_prop.at_night_color_mode);
		aaa_adj_set_contrast_value(&img_prop.contrast);
		aaa_adj_set_sharpness_value(&img_prop.sharpness);
		aaa_adj_set_2dnr_value(&img_prop.nr3d_level); //2dnr value is corresponding with 3dnr
		aaa_adj_set_3dnr_value(&img_prop.nr3d_level);
		aaa_adj_set_wdr_value(&img_prop.dwdr_level);
		//aaa_ae_setBLCLevel(img_prop.blc_level);
		/*
		slog.warn("[_change_img_color_ init] ");
		//slog.warn("[_change_img_color_] AdjStatus.Mirror = %d", AdjStatus.Mirror);
		slog.warn("[_change_img_color_] AdjStatus.Brightness = %d", img_prop.brightness*6);
		slog.warn("[_change_img_color_] AdjStatus.Saturation = %d", (img_prop.saturation+10)*128/20);
		slog.warn("[_change_img_color_] AdjStatus.Hue = %d", img_prop.hue*3/2);
		slog.warn("[_change_img_color_] AdjStatus.BW = %d", AdjStatus.BW);
		slog.warn("[_change_img_color_] AdjStatus.Constart = %d", (img_prop.contrast+10)*128/20);
		slog.warn("[_change_img_color_] AdjStatus.Sharpen = %d", (img_prop.sharpness+10)*5);
		slog.warn("[_change_img_color_] AdjStatus.NR_2D = %d", img_prop.nr3d_level);
		slog.warn("[_change_img_color_] AdjStatus.NR_3D = %d", img_prop.nr3d_level);
		slog.warn("[_change_img_color_] AdjStatus.WDR = %d",img_prop.dwdr_level);
		slog.warn("[_change_img_color_] ");
		*/
	} else {
		//******* ADJ *******//
		aaa_platform_init_rgb2Yuv(img_prop.at_day_mode);
		//aaa_adj_set_brightness(aaa_adj_set_brightness_value(&img_prop.brightness));
		//aaa_adj_set_saturation(aaa_adj_set_saturation_value(&img_prop.saturation));
		//aaa_adj_set_hue(aaa_adj_set_hue_value(&img_prop.hue));
		aaa_adj_set_blc();
		aaa_adj_set_yuv(aaa_adj_set_brightness_value(&img_prop.brightness), \
						aaa_adj_set_saturation_value(&img_prop.saturation), \
						aaa_adj_set_hue_value(&img_prop.hue), \
						aaa_adj_set_bw_value(!img_prop.at_day_mode && !img_prop.at_night_color_mode) \
		);
		aaa_adj_set_contrast(aaa_adj_set_contrast_value(&img_prop.contrast));

		aaa_adj_set_sharpness(aaa_adj_set_sharpness_value(&img_prop.sharpness));

		//3DNR
		aaa_adj_set_3dnr(aaa_adj_set_3dnr_value(&img_prop.nr3d_level));

		//2DNR
		aaa_adj_set_2dnr(aaa_adj_set_2dnr_value(&img_prop.nr3d_level)); //2dnr value is corresponding with 3dnr

		//WDR
		if(aaa_ae_BLC_isEnable()){ // Back_Light_WDR
			if(img_prop.blc_level == 3) aaa_adj_set_wdr(5);
			else if (img_prop.blc_level> 0) aaa_adj_set_wdr(4);
			else aaa_adj_set_wdr(aaa_adj_set_wdr_value(&img_prop.dwdr_level));
		}
		else{
			aaa_adj_set_wdr(aaa_adj_set_wdr_value(&img_prop.dwdr_level));
		}

		//Color Noise Reduction
		//aaa_adj_set_color_nr(aaa_adj_set_2dnr_value(&img_prop.nr3d_level));//Color_nr is corresponding with 2dnr

		/*
		slog.warn("[_change_img_color_ set] ");
		//slog.warn("[_change_img_color_] AdjStatus.Mirror = %d", AdjStatus.Mirror);
		slog.warn("[_change_img_color_] AdjStatus.Brightness = %d", img_prop.brightness*6);
		slog.warn("[_change_img_color_] AdjStatus.Saturation = %d", (img_prop.saturation+10)*128/20);
		slog.warn("[_change_img_color_] AdjStatus.Hue = %d", img_prop.hue*3/2);
		slog.warn("[_change_img_color_] AdjStatus.BW = %d", AdjStatus.BW);
		slog.warn("[_change_img_color_] AdjStatus.Constart = %d", (img_prop.contrast+10)*128/20);
		slog.warn("[_change_img_color_] AdjStatus.Sharpen = %d", (img_prop.sharpness+10)*5);
		slog.warn("[_change_img_color_] AdjStatus.NR_2D = %d", img_prop.nr3d_level);
		slog.warn("[_change_img_color_] AdjStatus.NR_3D = %d", img_prop.nr3d_level);
		slog.warn("[_change_img_color_] AdjStatus.WDR = %d",img_prop.dwdr_level);
		slog.warn("[_change_img_color_] ");
		*/
	}

	// Lens Shading Correction
	aaa_platform_set_vignette(&img_config);
	// Lens Distorion Correction
	aaa_platform_lens_distortion_correction(img_prop.iLensDewarpEnable);

}

int sensor_mode_init(void)
{
	char sensor_size[20] = {0};
	int type  = 0;
	json_t *obj = NULL, *json = NULL;

	su_prop_get(&json, PROP_PST_IMAGE_SENSOR_INFO);
	su_prop_run(&json);
    obj = json_object_get(json, PROP_PST_IMAGE_SENSOR_INFO);
    if (json_is_string(obj)){
		strncpy(sensor_size, json_string_value(obj), sizeof(sensor_size));
    } else {
        slog.error("Sensor info init error!");
    }

	if (strcmp(sensor_size, "20M") == 0)
		type = VIDEO_20M;
	if (strcmp(sensor_size, "5M") == 0)
		type = VIDEO_5M;
	if (strcmp(sensor_size, "4K") == 0)
		type = VIDEO_4K;
	
	prop_free(&json);

	return type;
}


int lens_type_init(void)
{
	int lens_type = 0;
	json_t *obj = NULL, *json = NULL;

	su_prop_get(&json, PROP_PST_IMAGE_LENS_DEWARP_ID);
	su_prop_run(&json);
    obj = json_object_get(json, PROP_PST_IMAGE_LENS_DEWARP_ID);
    if (json_is_integer(obj)) {
        lens_type = json_integer_value(obj);
    } else {
        slog.error("Lens init error!");
    }
	prop_free(&json);
	return lens_type;
}

int fps_mode_init(void)
{
	char fps_mode[20] = {0};
	int mode  = 0;
	json_t *obj = NULL, *json = NULL;

	su_prop_get(&json, PROP_PST_VIDEO_FPS_MODE);
	su_prop_run(&json);
    obj = json_object_get(json, PROP_PST_VIDEO_FPS_MODE);
    if (json_is_string(obj)){
		strncpy(fps_mode, json_string_value(obj), sizeof(fps_mode));
    } else {
        slog.error("fps mode init error!");
    }
	if (strcmp(fps_mode, "normal") == 0)
		mode = FPS_NORMAL;
	if (strcmp(fps_mode, "low") == 0)
		mode = FPS_LOW;

	prop_free(&json);

	return mode;
}


gpointer vs_3a_thread_run(gpointer data)
{
    slog.warn("%s start",__func__);
    //hal_init_lens(lens_callback_register);
    aaa_isp_setLensType(lens_type_init());
    aaa_isp_setSensorType(hal_get_board_sensor_type());
	aaa_isp_setSensorMode(sensor_mode_init());
    aaa_isp_setHDRMode(hal_get_image_hdr_level());
	aaa_isp_setFpsMode(fps_mode_init());
    //aaa_af_init(aaa_isp_getSensorType(), aaa_isp_getLensType());
    aaa_isp_loadSysDebugMode();
	aaa_isp_init = 1;
	pthread_mutex_init(&aaa_isp_mutex, NULL);
	pthread_cond_init(&aaa_isp_cond, NULL);
    while(1) {
        //slog.warn("%s rpthread_mutex_lock(&aaa_isp_mutex);
		pthread_cond_wait(&aaa_isp_cond, &aaa_isp_mutex);
		pthread_mutex_unlock(&aaa_isp_mutex);

		if(aaa_isp_flag & ISP_EVENT_SET_AE){
			AeStatus.AgcIdxReal = aaa_platform_set_sht_agc(AeStatus.AutoShtIdx, AeStatus.AutoAgcIdx);
			aaa_isp_setFlag(ISP_EVENT_SET_AE, 0);
		}
		
		usleep(5000);
    }
    slog.warn("%s exit",__func__);
    return NULL;
}


static pthread_t aaaDbg_id=0;
unsigned int aaa_debug_var[3];

void aaa_dbg_loop(void)
{
	char awbDbgFile[]="/tmp/aaa";
	int argv[3],argc;
	FILE *pfile = NULL;

	while(1) {
		usleep(500000);//500ms
		if (!access(awbDbgFile,F_OK)) {
			pfile = fopen(awbDbgFile, "r+");
			if (pfile) {
				argc=fscanf(pfile, "%d %d %d", &argv[0],&argv[1],&argv[2]);
				fclose(pfile);
				remove(awbDbgFile);
				slog.warn("argc:%d ,%d %d %d\n",argc,argv[0],argv[1],argv[2]);
				aaa_debug_var[0] = argv[0];
				aaa_debug_var[1] = argv[1];
				aaa_debug_var[2] = argv[2];				
			}
		}
	}
}

void init3aDbgThread(void)
{
	if(!aaaDbg_id) {
		slog.warn("%s",__func__);
		slog.warn("*****************test-----aaaDbg_id***********!\n");
		pthread_create(&aaaDbg_id, NULL, (void *)aaa_dbg_loop, (void *)NULL);
	}
}

