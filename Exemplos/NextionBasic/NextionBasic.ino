void setup() {
  // put your setup code here, to run once:

  delay(3000);

Serial.begin(115200);
Serial.println(" ");
Serial.println(" ");
Serial.println(" ");

delay(1000);

float count =0;
  int  temp=0;
String command = "";


  Serial.print("page 1");
  endNextionCommand();
  delay(1300);
  Serial.print("page 0");
  endNextionCommand();
  delay(1300);

  

delay(5000);
  while(1)
  {

     command = "temperaturein.txt=\""+String(count)+".0"+"\"";
    Serial.print(command);
    endNextionCommand();

    //delay(300);

    command = "temperatureout.txt=\""+String(temp)+".0"+"\"";
    Serial.print(command);
    endNextionCommand();

    //delay(300);

    command = "humidity.txt=\""+String(count)+".0"+"\"";
    Serial.print(command);
    endNextionCommand();

    //delay(300);

    command = "humidityout.txt=\""+String(temp)+".0"+"\"";
    Serial.print(command);
    endNextionCommand();

    delay(300);
    count = count+0.1;
    temp ++;

    if(count>90)
      count =0;
    if(temp>90)
      temp=0;

   // Serial.println();
  }


}

void loop() {
  // put your main code here, to run repeatedly:

}


void endNextionCommand()
{
  Serial.write(0xff);
  Serial.write(0xff);
  Serial.write(0xff);
}