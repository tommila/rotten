/*
 * Copyright 2011-2023 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bgfx/blob/master/LICENSE
 */

#include "common.sh"
uniform vec4 u_color;

void main()
{
	gl_FragColor = mix(vec4(0.5,0.5,0.5,1.0),u_color,0.5);
}
