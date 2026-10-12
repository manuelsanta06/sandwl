#version 320 es
precision mediump float;
in vec4 ourColor;
out vec4 FragColor;

void main(){
  FragColor=vec4(ourColor);
}
