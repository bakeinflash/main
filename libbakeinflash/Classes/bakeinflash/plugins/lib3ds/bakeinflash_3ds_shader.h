// shader_fs.h

// This source code has been donated to the Public Domain.  Do
// whatever you want with it.

// Lib3ds plugin implementation for bakeinflash library

namespace bakeinflash
{

	// fs shader
	static const char* s_3ds_shader[2] = {

	"uniform sampler2D		textureMap;"
	"uniform sampler2D		bumpMap;"
	"uniform sampler2DShadow	shadowMap;"

	"uniform float	texturePercent;"
	"uniform float	bumpPercent;"
	//uniform float	dt;

	"varying vec3	normal;"
	"varying vec3	lightVec;"
	"varying vec3	eyeVec;"
	//varying vec3	spotVec;
	"varying vec4	shadowCoord;"

	//const float cos_outer_cone_angle = 0.92; // 45 degrees

	"void main (void)"
	"{"
	"	vec4 unitVec = vec4(1.0,1.0,1.0,1.0);"
		
	"	vec4 texColor = unitVec - texturePercent * (unitVec - texture2D(textureMap, gl_TexCoord[0].xy));"
	"	vec4 Ma = gl_FrontMaterial.ambient + texturePercent * (unitVec - gl_FrontMaterial.ambient);"
	"	vec4 Md = gl_FrontMaterial.diffuse + texturePercent * (unitVec - gl_FrontMaterial.diffuse);"
								
	"	vec3 L = normalize(lightVec);"
		//vec3 D = normalize(spotVec);
		
		// shadow with PCF
		/*vec4 shadow =
			(shadow2DProj(shadowMap, shadowCoord) +
			shadow2DProj(shadowMap, shadowCoord + vec4(dt,0.0,0.0,0.0)) +
			shadow2DProj(shadowMap, shadowCoord + vec4(dt,dt,0.0,0.0)) +
			shadow2DProj(shadowMap, shadowCoord + vec4(0.0,dt,0.0,0.0))) * 0.25;*/
	"	vec4 shadow = shadow2DProj(shadowMap, shadowCoord);"
		//float shadow = shadow2D(shadowMap, shadowCoord).r;
		
		// ambient component
	"	vec4 vAmbient = (gl_FrontLightModelProduct.sceneColor * Ma) + "
	"			(gl_LightSource[0].ambient * Ma);"
	"	vec4 final_color = vAmbient * texColor;"
		
		//float cos_cur_angle = dot(-L, D);
		//float cos_inner_cone_angle = gl_LightSource[0].spotCosCutoff;
		//float cos_inner_minus_outer_angle = cos_inner_cone_angle - cos_outer_cone_angle;
		
	"	float spot = 1.0;"
		//float spot = clamp((cos_cur_angle - cos_outer_cone_angle) / 
		//	cos_inner_minus_outer_angle, 0.0, 1.0);

	"	vec3 N;"
	"	if (bumpPercent > 0.0)"
	"	{"
	"		vec3 tmpVec = texture2D(bumpMap, gl_TexCoord[0].xy).xyz * 2.0 - 1.0;"
			//N = normalize(vec3(tmpVec.xy * bumpPercent * 5.0, tmpVec.z));
	"		N = normalize(vec3(tmpVec.xy * bumpPercent * 2.0, tmpVec.z));"
	"	}"
	"	else"
	"	{	"
	"		N = normalize(normal);"
	"	}"
		
		// diffuse component
	"	float diffuse = max(0.0, dot(L, N));"
	"	if (diffuse > 0.0)"
	"	{"
	"		vec4 vDiffuse = gl_LightSource[0].diffuse * Md * diffuse * spot;"
			
	"		vec3 E = normalize(eyeVec);"
			
			// specular component
	"		float specular = pow(clamp(dot(reflect(-L, N), E), 0.0, 1.0), gl_FrontMaterial.shininess);"
	"		vec4 vSpecular = (1.0 - texturePercent) * gl_LightSource[0].specular * "
	"				gl_FrontMaterial.specular * specular * spot;"
			//vec4 vSpecular = gl_LightSource[0].specular * gl_FrontMaterial.specular * specular;
			
			//final_color += vDiffuse * texColor + vSpecular;
	"		final_color += (0.2 + 0.8 * shadow) * vDiffuse * texColor + vSpecular * shadow;"
	"	}"

		//gl_FragColor = final_color * shadow;
	"	gl_FragColor = final_color;"
	"}"
	,

	//
	// vs shader
	//

	"uniform float	bumpPercent;"

	"varying vec3	normal;"
	"varying vec3	lightVec;"
	"varying vec3	eyeVec;"
	//varying vec3	spotVec;
	"varying vec4	shadowCoord;"

	"void main()"
	"{	"
	"	gl_Position = ftransform();"
		
		// texture(bump) coordinates
	"	gl_TexCoord[0].xy = gl_MultiTexCoord0.xy;"
		

	"	vec3 vVertex = vec3(gl_ModelViewMatrix * gl_Vertex);"

			// shadow texcoords
	"    shadowCoord = gl_TextureMatrix[2] * gl_ModelViewMatrix * gl_Vertex;"
	    
	"	if (bumpPercent > 0.0)"
	"	{"
	"		vec3 tangent;"
	"		vec3 c1 = cross(gl_Normal, vec3(0.0, 0.0, 1.0));"
	"		vec3 c2 = cross(gl_Normal, vec3(0.0, 1.0, 0.0));"
	"		if(length(c1)>length(c2))"
	"		{"
	"			tangent = c1;	"
	"		}"
	"		else"
	"		{"
	"			tangent = c2;	"
	"		}"

	"		vec3 n = normalize(gl_NormalMatrix * gl_Normal);"
			//TODO:
			//vec3 n = normalize(gl_NormalMatrix * gl_MultiTexCoord3.xyz);
			//vec3 t = normalize(gl_NormalMatrix * gl_MultiTexCoord1.xyz);
			//vec3 b = normalize(gl_NormalMatrix * gl_MultiTexCoord2.xyz);
	"		vec3 t = normalize(gl_NormalMatrix * tangent);"
	"		vec3 b = cross(n, t);"

	"		vec3 tmpVec = vec3(gl_LightSource[0].position.xyz - vVertex);"
		
	"		lightVec.x = dot(tmpVec, t);"
	"		lightVec.y = dot(tmpVec, b);"
	"		lightVec.z = dot(tmpVec, n);"
			
	"		tmpVec = -vVertex;"
	"		eyeVec.x = dot(tmpVec, t);"
	"		eyeVec.y = dot(tmpVec, b);"
	"		eyeVec.z = dot(tmpVec, n);"
			
	/*		tmpVec = gl_LightSource[0].spotDirection;
			spotVec.x = dot(tmpVec, t);
			spotVec.y = dot(tmpVec, b);
			spotVec.z = dot(tmpVec, n);*/
	"	}"
	"	else"
	"	{"
	"		normal = gl_NormalMatrix * gl_Normal;"
	"		lightVec = vec3(gl_LightSource[0].position.xyz - vVertex);"
	"		eyeVec = -vVertex;"
	//		spotVec = gl_LightSource[0].spotDirection;
	"	}"	
	"}"

	};	// end of s_shader


}
