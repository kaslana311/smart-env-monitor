clearvars; % 清空工作区

% 大地坐标系下目标方位
targetyaw = 0;
targetpitch = 0;
% 当前的云台姿态
nowpitch = 0;
nowyaw = 0;
nowroll = 0;
nowmotorpitch = pi/12;
nowmotoryaw = 0;% 由于只需要输出误差, 所以默认当前云台方向为 0

% 大地坐标系下基准方向, 即 Yaw=Pitch=Roll=0
x=[1,0,0.5;1,0,-0.5]';
plot3(x(1,1:end),x(2,1:end),x(3,1:end),'blue');
grid on;
hold on;
plot3([x(1,1),0],[x(2,1),0],[x(3,1),0],'blue');
plot3([x(1,2),0],[x(2,2),0],[x(3,2),0],'blue');
tempx=(x(1:end,1)+x(1:end,2))/2;
plot3([tempx(1),0],[tempx(2),0],[tempx(3),0],'blue');
title('大地坐标系下的坐标信息');

% 旋转顺序 Yaw-Pitch-Roll 旋转方式内旋
% 在大地坐标系下描述目标, 只有两个自由度
Ryaw=[cos(targetyaw),-sin(targetyaw),0;sin(targetyaw),cos(targetyaw),0;0,0,1];
Rpitch=[cos(targetpitch),0,-sin(targetpitch);0,1,0;sin(targetpitch),0,cos(targetpitch)];
x1=Ryaw*Rpitch*x;
% 大地坐标系下目标方向
plot3(x1(1,1:end),x1(2,1:end),x1(3,1:end),'red');
plot3([x1(1,1),0],[x1(2,1),0],[x1(3,1),0],'red');
plot3([x1(1,2),0],[x1(2,2),0],[x1(3,2),0],'red');
tempx=(x1(1:end,1)+x1(1:end,2))/2;
plot3([tempx(1),0],[tempx(2),0],[tempx(3),0],'red');

% 计算当前云台姿态坐标
RyawGimbal=[cos(nowyaw),-sin(nowyaw),0;sin(nowyaw),cos(nowyaw),0;0,0,1];
RpitchGimbal=[cos(nowpitch),0,sin(nowpitch);0,1,0;-sin(nowpitch),0,cos(nowpitch)];
RrollGimbal=[1,0,0;0,cos(nowroll),-sin(nowroll);0,sin(nowroll),cos(nowroll)];
RGimbalFromGround = RyawGimbal*RpitchGimbal*RrollGimbal;
xGimbal = RGimbalFromGround*x;
plot3(xGimbal(1,1:end),xGimbal(2,1:end),xGimbal(3,1:end),'cyan');
plot3([xGimbal(1,1),0],[xGimbal(2,1),0],[xGimbal(3,1),0],'cyan');
plot3([xGimbal(1,2),0],[xGimbal(2,2),0],[xGimbal(3,2),0],'cyan');
tempx=(xGimbal(1:end,1)+xGimbal(1:end,2))/2;
plot3([tempx(1),0],[tempx(2),0],[tempx(3),0],'cyan');
% 计算当前底盘姿态
up = RGimbalFromGround*[0;1;0];% 云台坐标系下 Pitch 轴旋转轴(x 轴)方向
targetpitchx=-nowmotorpitch;
RchassisPitch=[up(1)*up(1)*(1-cos(targetpitchx))+cos(targetpitchx),up(1)*up(2)*(1-cos(targetpitchx))+up(3)*sin(targetpitchx),up(1)*up(3)*(1-cos(targetpitchx))-up(2)*sin(targetpitchx);
               up(1)*up(2)*(1-cos(targetpitchx))-up(3)*sin(targetpitchx),up(2)*up(2)*(1-cos(targetpitchx))+cos(targetpitchx),up(2)*up(3)*(1-cos(targetpitchx))+up(1)*sin(targetpitchx);
               up(1)*up(3)*(1-cos(targetpitchx))+up(2)*sin(targetpitchx),up(2)*up(3)*(1-cos(targetpitchx))-up(1)*sin(targetpitchx),up(3)*up(3)*(1-cos(targetpitchx))+cos(targetpitchx)];
RChassisFromGround = RchassisPitch*RGimbalFromGround;
x0=RChassisFromGround*x;
plot3(x0(1,1:end),x0(2,1:end),x0(3,1:end),'green');
plot3([x0(1,1),0],[x0(2,1),0],[x0(3,1),0],'green');
plot3([x0(1,2),0],[x0(2,2),0],[x0(3,2),0],'green');
tempx=(x0(1:end,1)+x0(1:end,2))/2;
plot3([tempx(1),0],[tempx(2),0],[tempx(3),0],'green');

%从底盘基准坐标系到目标的旋转
figure(2);
xx=[1,0,0.5;1,0,-0.5]';
%绘制起始观测坐标
plot3(xx(1,1:end),xx(2,1:end),xx(3,1:end),'green');
grid on;
hold on;
plot3([xx(1,1),0],[xx(2,1),0],[xx(3,1),0],'green');
plot3([xx(1,2),0],[xx(2,2),0],[xx(3,2),0],'green');
tempx=(xx(1:end,1)+xx(1:end,2))/2;
plot3([tempx(1),0],[tempx(2),0],[tempx(3),0],'green');
title('底盘坐标系下的坐标信息');
%绘制大地坐标系坐标
x2=(RchassisPitch*RyawGimbal*RpitchGimbal*RrollGimbal)\xx;
plot3(x2(1,1:end),x2(2,1:end),x2(3,1:end),'blue');
plot3([x2(1,1),0],[x2(2,1),0],[x2(3,1),0],'blue');
plot3([x2(1,2),0],[x2(2,2),0],[x2(3,2),0],'blue');
tempx=(x2(1:end,1)+x2(1:end,2))/2;
plot3([tempx(1),0],[tempx(2),0],[tempx(3),0],'blue');
%给出底盘姿态欧拉角(相对大地坐标系)
Chassis=rotm2eul(inv(RchassisPitch*RyawGimbal*RpitchGimbal*RrollGimbal));
ChassisYaw=Chassis(1);
ChassisPitch=Chassis(2);
ChassisRoll=Chassis(3);

%绘制目标坐标
uy=x2(1:end,1)-x2(1:end,2);
targetyawx=-targetyaw;
R1=[uy(1)*uy(1)*(1-cos(targetyawx))+cos(targetyawx),uy(1)*uy(2)*(1-cos(targetyawx))+uy(3)*sin(targetyawx),uy(1)*uy(3)*(1-cos(targetyawx))-uy(2)*sin(targetyawx);
    uy(1)*uy(2)*(1-cos(targetyawx))-uy(3)*sin(targetyawx),uy(2)*uy(2)*(1-cos(targetyawx))+cos(targetyawx),uy(2)*uy(3)*(1-cos(targetyawx))+uy(1)*sin(targetyawx);
    uy(1)*uy(3)*(1-cos(targetyawx))+uy(2)*sin(targetyawx),uy(2)*uy(3)*(1-cos(targetyawx))-uy(1)*sin(targetyawx),uy(3)*uy(3)*(1-cos(targetyawx))+cos(targetyawx)];
ur=(x2(1:end,1)+x2(1:end,2))/2;
up=cross(ur,uy);
targetpitchx=-targetpitch;
R2=[up(1)*up(1)*(1-cos(targetpitchx))+cos(targetpitchx),up(1)*up(2)*(1-cos(targetpitchx))+up(3)*sin(targetpitchx),up(1)*up(3)*(1-cos(targetpitchx))-up(2)*sin(targetpitchx);
    up(1)*up(2)*(1-cos(targetpitchx))-up(3)*sin(targetpitchx),up(2)*up(2)*(1-cos(targetpitchx))+cos(targetpitchx),up(2)*up(3)*(1-cos(targetpitchx))+up(1)*sin(targetpitchx);
    up(1)*up(3)*(1-cos(targetpitchx))+up(2)*sin(targetpitchx),up(2)*up(3)*(1-cos(targetpitchx))-up(1)*sin(targetpitchx),up(3)*up(3)*(1-cos(targetpitchx))+cos(targetpitchx)];
RTargetFromChassis = R1*R2;
xx1=RTargetFromChassis*x2;
plot3(xx1(1,1:end),xx1(2,1:end),xx1(3,1:end),'red');
plot3([xx1(1,1),0],[xx1(2,1),0],[xx1(3,1),0],'red');
plot3([xx1(1,2),0],[xx1(2,2),0],[xx1(3,2),0],'red');
tempx=(xx1(1:end,1)+xx1(1:end,2))/2;
plot3([tempx(1),0],[tempx(2),0],[tempx(3),0],'red');

%标准的旋转矩阵转欧拉角, 这里 xx2 就是 xx1
R=R1*R2/(RchassisPitch*RyawGimbal*RpitchGimbal*RrollGimbal);
s=rotm2eul(R);
RealYaw=s(1);
RealPitch=-s(2);
RealRoll=s(3);
Ryaw=[cos(RealYaw),-sin(RealYaw),0;sin(RealYaw),cos(RealYaw),0;0,0,1];
Rpitch=[cos(RealPitch),0,-sin(RealPitch);0,1,0;sin(RealPitch),0,cos(RealPitch)];
Rroll=[1,0,0;0,cos(RealRoll),-sin(RealRoll);0,sin(RealRoll),cos(RealRoll)];
xx2=Ryaw*Rpitch*Rroll*xx;
FinalPosWithStandardrotm2eul=(xx2(1:end,1)+xx2(1:end,2))/2;
disp(FinalPosWithStandardrotm2eul);

% 根据旋转矩阵计算需要转动的欧拉角
test=R*[1;0;0];
%计算需要转动的Yaw
yaw=atan(test(2)/test(1));
if((test(2)>0)&&(test(1)<0))
    yaw=yaw+pi;
elseif ((test(1)<0)&&(test(2)<0))
    yaw=yaw-pi;
end
Ryaw=[cos(yaw),-sin(yaw),0;sin(yaw),cos(yaw),0;0,0,1];
xx3=Ryaw*xx;
plot3(xx3(1,1:end),xx3(2,1:end),xx3(3,1:end),'yellow');
plot3([xx3(1,1),0],[xx3(2,1),0],[xx3(3,1),0],'yellow');
plot3([xx3(1,2),0],[xx3(2,2),0],[xx3(3,2),0],'yellow');
tempx=(xx3(1:end,1)+xx3(1:end,2))/2;
plot3([tempx(1),0],[tempx(2),0],[tempx(3),0],'yellow');
%计算需要转动的Pitch
pitch=atan(test(3)/sqrt((test(1)*test(1)+test(2)*test(2))));
Rpitch=[cos(pitch),0,-sin(pitch);0,1,0;sin(pitch),0,cos(pitch)];
xx4=Ryaw*Rpitch*xx;
plot3(xx4(1,1:end),xx4(2,1:end),xx4(3,1:end),'black');
plot3([xx4(1,1),0],[xx4(2,1),0],[xx4(3,1),0],'black');
plot3([xx4(1,2),0],[xx4(2,2),0],[xx4(3,2),0],'black');
tempx=(xx4(1:end,1)+xx4(1:end,2))/2;
plot3([tempx(1),0],[tempx(2),0],[tempx(3),0],'black');
FinalPosWithMyOwnCalc=tempx;
disp(FinalPosWithMyOwnCalc);

%输出计算结果
fprintf("大地坐标系下目标方位:\nYaw:%fdeg\tPitch:%fdeg\r\n", targetyaw/pi*180, targetpitch/pi*180);
fprintf("底盘坐标系相对大地坐标系姿态:\nPitch:%fdeg\tYaw:%fdeg\tRoll:%fdeg\r\n", ChassisPitch/pi*180,ChassisYaw/pi*180,ChassisRoll/pi*180);
fprintf("转到目标所需的云台姿态:\nYaw:%fdeg\tPitch:%fdeg\r\n", yaw/pi*180,pitch/pi*180);
fprintf("电机角度闭环误差:\nYaw:%fdeg\tPitch:%fdeg\r\n",(yaw-nowmotoryaw)/pi*180,(pitch-nowmotorpitch)/pi*180);
%说明绘图因素
fprintf("第一张图代表：大地坐标系下的底盘姿态与目标方位\n");
fprintf("蓝色三角形代表\t基础大地坐标系视角\n");
fprintf("青色三角形代表\t当前云台坐标系视角\n");
fprintf("红色三角形代表\t大地坐标系下目标方位\n");
fprintf("绿色三角形代表\t大地坐标系下地盘姿态\r\n");
fprintf("第二张图代表：底盘坐标系下的目标方位与云台姿态\n");
fprintf("绿色三角形代表\t基础底盘坐标系 视角\n");
fprintf("蓝色三角形代表\t基础大地坐标系视角\n");
fprintf("红色三角形代表\t底盘坐标系下目标方位\n");
fprintf("黄色三角形代表\t底盘坐标系下Yaw轴旋转后的云台姿态\n");
fprintf("黑色三角形代表\t底盘坐标系下Yaw、Pitch轴旋转后的云台姿态\r\n");

% 上述内容只作为思路的探究, 实际实现时发现不论用什么手段来处理, 上述计算方法的计算量都很大, 所以考虑使用四元数来等价地进行旋转操作
% 注意本段程序与上面的不同的点在于, HERO 战队步兵机器人的云台姿态坐标系是左手系, 为了贴合实际代码, 本段程序输入输出采用左手系来描述姿态

% 四元数计算时将描述姿态的左手系转换为右手系——z轴正方向向下, 但是旋转操作的正方向按照原始左手系方向
% 计算当前云台姿态坐标
QGimbalFromGround = angle2quat(-nowyaw,nowpitch,nowroll);
% 计算当前底盘姿态
p1 = quatmultiply(quatmultiply(QGimbalFromGround,[0,0,1,0]),quatinv(QGimbalFromGround)); % 云台坐标系下 Pitch 轴旋转轴方向
QchassisPitch = [cos(nowmotorpitch/2), p1(2)*sin(nowmotorpitch/2),p1(3)*sin(nowmotorpitch/2),p1(4)*sin(nowmotorpitch/2)];% 构造定轴旋转四元数
QChassisFromGround = quatmultiply(QchassisPitch,QGimbalFromGround);
% 计算底盘坐标系下目标坐标
qyaw = quatmultiply(quatmultiply(quatinv(QChassisFromGround),[0,-1,0,0]),QChassisFromGround); % Yaw 轴的旋转轴是现在的 -z 方向
Q1 = [cos(targetyaw/2), qyaw(2)*sin(targetyaw/2),qyaw(3)*sin(targetyaw/2),qyaw(4)*sin(targetyaw/2)]; % 构造定轴旋转四元数
qpitch = quatmultiply(quatmultiply(quatinv(QChassisFromGround),[0,0,1,0]),QChassisFromGround);
Q2 = [cos(targetpitch/2), qpitch(2)*sin(targetpitch/2),qpitch(3)*sin(targetpitch/2),qpitch(4)*sin(targetpitch/2)]; % 构造定轴旋转四元数
QTargetFromChassis = quatmultiply(Q1,Q2);
% 计算底盘坐标系到目标的旋转矩阵
Q = quatmultiply(QTargetFromChassis,quatinv(QChassisFromGround));
% 获得目标相对底盘坐标系的欧拉角
P = quatmultiply(quatmultiply(Q,[0,0,0,-1]),quatinv(Q));
pitch = -asin(P(2)/sqrt(P(2)^2+P(3)^2+P(4)^2));
yaw = atan(P(3)/P(4));
Error = [rad2deg(yaw-nowmotoryaw),rad2deg(pitch-nowmotorpitch)];
disp(Error);
