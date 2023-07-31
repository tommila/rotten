$input v_texcoord0
/*
 * Copyright 2011-2023 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bgfx/blob/master/LICENSE
 */

#include "common.sh"

SAMPLER2D(s_texColor,  0);

void main()
{
	vec4 tex_color = toLinear(texture2D(s_texColor, v_texcoord0));
	gl_FragColor = mix(vec4(0.5,0.5,0.5,1.0),tex_color,0.5);
}
