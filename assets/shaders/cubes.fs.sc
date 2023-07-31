$input v_texcoord0
/*
 * Copyright 2011-2023 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bgfx/blob/master/LICENSE
 */

#include "common.sh"

SAMPLER2D(s_texColor, 0);
uniform vec4 u_color;

void main()
{
	vec4 tex_color = toLinear(texture2D(s_texColor, v_texcoord0));
	gl_FragColor = tex_color * u_color;
}
