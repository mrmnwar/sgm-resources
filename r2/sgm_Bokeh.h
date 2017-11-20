#ifndef SGM_BOKEH_H_INCLUDED
#define SGM_BOKEH_H_INCLUDED

uniform half uBokehBias;
uniform float uBokehGain;
uniform half uBokehMaxBlur;
uniform float uBokehTreshold;
uniform half uDepthBlurSize;
uniform half uPentagonFeather;
uniform half uBokehFringe;
uniform float uBokehNoiseAmount;

#define BOKEH_NOISE
//#define BOKEH_PENTAGON//use pentagonal shape
#define BOKEH_RINGS int(3)
#define BOKEH_SAMPLES int(3)

half penta(float2 texcoord)//pentagonal shape
{

	half scale = half(BOKEH_RINGS)-1.3;
	float4 HS0 = float4( 1,            0,            0,  1);
	float4 HS1 = float4( 0.309016994,  0.951056516,  0,  1);
	float4 HS2 = float4(-0.809016994,  0.587785252,  0,  1);
	float4 HS3 = float4(-0.809016994, -0.587785252,  0,  1);
	float4 HS4 = float4( 0.309016994, -0.951056516,  0,  1);
	float4 HS5 = float4( 0,            0,            1,  1);
	
	float4 one=float4(1.0,1.0,1.0,1.0);
	
	float4 P = float4(texcoord,scale,scale);
	
	float4 dist=float4(0.0,0.0,0.0,0.0);
	half inorout=-4.0;
	
	dist.x = dot(P,HS0);
	dist.y = dot(P,HS1);
	dist.z = dot(P,HS2);
	dist.w = dot(P,HS3);
	
	dist = smoothstep(-uPentagonFeather,uPentagonFeather,dist);
	
	inorout += dot(dist,one);
	
	dist.x = dot(P,HS4);
	dist.y = HS5.w - abs( P.z );
	
	dist = smoothstep(-uPentagonFeather,uPentagonFeather,dist);
	inorout += dist.x;
	
	return clamp(inorout,0.0,1.0);//saturate
}

half bdepth(float2 texcoord)//blurring depth
{
	half d=0.0;
	half kernel[9]
	float2 offset[9];
	
	float2 wh = float2(screen_res.z,screen_res.w)*uDepthBlurSize;
	
	offset[0] = float2(-wh.x,-wh.y);
	offset[1] = float2( 0.0, -wh.y);
	offset[2] = float2( wh.x,-wh.y);
	
	offset[3] = float2(-wh.x, 0.0);
	offset[4] = float2( 0.0,  0.0);
	offset[5] = float2( wh.x, 0.0);
	
	offset[6] = float2(-wh.x, wh.y);
	offset[7] = float2( 0.0,  wh.y);
	offset[8] = float2( wh.x, wh.y);
	
	kernel[0] = 1.0/16.0; 	kernel[1] = 2.0/16.0; 	kernel[2] = 1.0/16.0;
	kernel[3] = 2.0/16.0; 	kernel[4] = 4.0/16.0; 	kernel[5] = 2.0/16.0;
	kernel[6] = 1.0/16.0; 	kernel[7] = 2.0/16.0; 	kernel[8] = 1.0/16.0;
	
	for (int i = 0; i < 9; i++)
	{
		half tmp = tex2D(sPosition,texcoord + offset[i]).z;
		d += tmp * kernel[i];
	}
	
	return d;
}

half3 color(float2 texcoord, half blur)//processing the sample
{
	half3 col=half3(0.0,0.0,0.0);
	
	//MAYBE tex2Dlod
	col.r = tex2D(sScene,texcoord + float2(0.0,1.0)*screen_res.zw*uBokehFringe*blur).r;
	col.g = tex2D(sScene,texcoord + float2(-0.866,-0.5)*screen_res.zw*uBokehFringe*blur).g;
	col.b = tex2D(sScene,texcoord + float2(0.866,-0.6)*screen_res.zw*uBokehFringe*blur).b;
	
	float lum = dot(col,LUMINANCE_VECTOR);
	float tresh = max((lum-uBokehTreshold)*uBokehGain,0.0);
	return col+lerp(half3(0.0,0.0,0.0),col,tresh*blur);
}

float2 rand(float2 texcoord)//generating noise/pattern texture for dithering
{
	float noiseX = ((frac(1.0-texcoord.x*(screen_res.x/2.0))*0.25)+(frac(texcoord.y*(screen_res.y/2.0))*0.75))*2.0-1.0;
	float noiseY = ((frac(1.0-texcoord.x*(screen_res.x/2.0))*0.75)+(frac(texcoord.y*(screen_res.y/2.0))*0.25))*2.0-1.0;
	
	#ifdef BOKEH_NOISE
		noiseX = clamp(frac(sin(dot(texcoord,float2(12.9898,78.233)))*43758.5453),0,1)*2-1;
		noiseY = clamp(frac(sin(dot(texcoord,float2(12.9898,78.233)*2))*43758.5453),0,1)*2-1;
	#endif	
	return float2(noiseX,noiseY);
}

half3 bokeh(float2 texcoord, float blur)
{
	
	//calculation of pattern for dithering
	
	float2 noise = rand(texcoord)*uBokehNoiseAmount*blur;
	
	//getting blur x and y step factor
	
	float w = screen_res.z * blur * uBokehMaxBlur + noise.x;
	float h = screen_res.w * blur * uBokehMaxBlur + noise.y;
	
	//calculation of final color
	
	half3 col = half3(0.0,0.0,0.0);
	
	if (blur < 0.05)//some optimization thingy
	{
		col = tex2D(sScene,texcoord).rgb;
	}
	else
	{
		col = tex2D(sScene,texcoord).rgb;
		half s=1.0;
		int ringsamples;
		
		for (int i = 1; i <= BOKEH_RINGS; i++)

		{

			ringsamples = i * BOKEH_SAMPLES;
			
			for (int j = 0; j < ringsamples; j++)
			{
				float step = PI*2.0/float(ringsamples);
				float pw = (cos(float(j)*step)*float(i));
				float ph = (sin(float(j)*step)*float(i));
				half p=1.0;
				#ifdef BOKEH_PENTAGON

					p = penta(float2(pw,ph));
				#endif	
				col += color(texcoord + float2(pw*w,ph*h),blur)*lerp(1.0,(float(i))/(float(BOKEH_RINGS)),uBokehBias)*p;	

				s += 1.0*lerp(1.0,(float(i))/(float(BOKEH_RINGS)),uBokehBias)*p;
			}
		}
		col/=s;//divide by sample count
	}
	return col;
}

#endif