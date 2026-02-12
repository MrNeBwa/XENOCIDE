#version 330 core
in float pColor;
out vec4 FragColor;
void main()
{
  FragColor = vec4(pColor, pColor, pColor, 1.0f);
};

