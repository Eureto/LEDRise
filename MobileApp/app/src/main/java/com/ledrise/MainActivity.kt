package com.ledrise

import android.content.SharedPreferences
import android.os.Bundle
import android.util.Log
import android.widget.Button
import android.widget.ImageButton
import android.widget.TimePicker
import androidx.activity.enableEdgeToEdge
import androidx.appcompat.app.AppCompatActivity
import androidx.core.view.ViewCompat
import androidx.core.view.WindowInsetsCompat
import android.widget.NumberPicker
import android.widget.TextView
import android.widget.Toast
import androidx.core.content.ContextCompat
import androidx.lifecycle.lifecycleScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext
import okhttp3.OkHttpClient
import okhttp3.Request
import java.io.IOException
import java.net.InetAddress
class MainActivity : AppCompatActivity() {

    private lateinit var sharedPreferences: SharedPreferences
    val deviceAddress: String = "192.168.0.150"
    var preMinutePicker: NumberPicker? = null
    var timePicker: TimePicker? = null
    var setButton: Button? = null
    var deleteButton: Button? = null
    var settingsButton: ImageButton? = null
    private val client = OkHttpClient()
    var onOffLedButton: Button? = null
    var textInfo: TextView? = null
    var isLedOn: Boolean = false
    var brightnessSlider: com.google.android.material.slider.Slider? = null


    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        enableEdgeToEdge()
        setContentView(R.layout.activity_main)
        ViewCompat.setOnApplyWindowInsetsListener(findViewById(R.id.main)) { v, insets ->
            val systemBars = insets.getInsets(WindowInsetsCompat.Type.systemBars())
            v.setPadding(systemBars.left, systemBars.top, systemBars.right, systemBars.bottom)
            insets
        }
        sharedPreferences = getSharedPreferences("AlarmSettings", MODE_PRIVATE)

        timePicker = findViewById<TimePicker>(R.id.timePicker)
        timePicker?.setIs24HourView(true)

        preMinutePicker = findViewById<NumberPicker>(R.id.minutePicker)

        // Configure the NumberPicker for minutes
        preMinutePicker?.minValue = 0
        preMinutePicker?.maxValue = 160
        preMinutePicker?.wrapSelectorWheel = true // Allows continuous scrolling (59 -> 0 and 0 -> 59)
        //preMinutePicker?.value = 15 // Example: Set initial value to 60 minutes

        // Load the saved pre-alarm value from SharedPreferences
        val sharedPreferences: SharedPreferences = getSharedPreferences("AlarmSettings", MODE_PRIVATE)
        val savedPreAlarmMinutes = sharedPreferences.getInt("preAlarmMinutes", 15)
        preMinutePicker?.value = savedPreAlarmMinutes


        textInfo = findViewById<TextView>(R.id.textViewSetInfo)

        setButton = findViewById<Button>(R.id.buttonSet)
        setButton?.setOnClickListener {
            onSetButtonClick()
        }
        deleteButton = findViewById<Button>(R.id.buttonDeleteAlarm)
        deleteButton?.setOnClickListener{
            onDeleteButtonClick()

        }
        settingsButton = findViewById<ImageButton>(R.id.buttonSettings)
        settingsButton?.setOnClickListener{
            //Open settings screen
            val intent = android.content.Intent(this, SettingsActivity::class.java)
            startActivity(intent)
        }
        onOffLedButton = findViewById(R.id.buttonTurnOnOffLed)
        onOffLedButton?.setOnClickListener{
            turnOnOffLed()
        }
        brightnessSlider = findViewById(R.id.brightnessSlider)
        brightnessSlider?.addOnChangeListener { _, value, _ ->
           setLedBrightness(value.toInt())
        }

        // After initialization check if alarm is set
        checkStatusOfAlarm()
        //checkLedOnOffState()
    }

    override fun onDestroy()
    {
        super.onDestroy()
        val sharedPreferences: SharedPreferences = getSharedPreferences("AlarmSettings", MODE_PRIVATE)
        val editor: SharedPreferences.Editor = sharedPreferences.edit()

        val preAlarmMinutes = preMinutePicker?.value.toString().toIntOrNull() ?: 15 // Default to 15 if parsing fails
        editor.putInt("preAlarmMinutes", preAlarmMinutes)
        editor.apply()

        Toast.makeText(this, "Settings saved", Toast.LENGTH_SHORT).show()
    }

    fun setLedBrightness(value: Int){
        val url = "http://${deviceAddress}/setBrightness?value=$value"
        lifecycleScope.launch {
            try {
                val responseBody = sendGetRequest(url)

                if(responseBody != null && responseBody.equals("Invalid brightness value. Must be between 0 and 255.")){
                    textInfo?.text = "Invalid brightness value. Must be between 0 and 255."
                }
            }
            catch(e: Exception){
                textInfo?.text = "An error occurred 1"
            }
        }

    }

    fun turnOnOffLed(){
        val url = "http://${deviceAddress}/turnOnOffLed"
        lifecycleScope.launch {
            try {
                val responseBody = sendGetRequest(url)

                if(responseBody != null){
                     if(responseBody.equals("LED turned on")){
                         textInfo?.text = "LED has been turned on"
                         isLedOn = true
                         val redColor = ContextCompat.getColor(this@MainActivity, R.color.red)
                         onOffLedButton?.setBackgroundColor(redColor)
                         onOffLedButton?.text = "Turn off LED"

                     }else if(responseBody.equals("LED turned off")){
                         textInfo?.text = "LED has been turned off"
                         isLedOn = false
                         val greenColor = ContextCompat.getColor(this@MainActivity, R.color.green)
                         onOffLedButton?.setBackgroundColor(greenColor)
                         onOffLedButton?.text = "Turn on LED"

                     }
                }
            }
            catch(e: Exception){
                textInfo?.text = "An error occurred 2"
            }
        }
    }

    fun checkLedOnOffState(){
        val url = "http://${deviceAddress}/ledStateStatus"
        lifecycleScope.launch {
            try {
                val responseBody = sendGetRequest(url)

                if(responseBody != null){
                    if(responseBody.equals("ON")){
                        isLedOn = true
                        onOffLedButton?.text = "Turn off LED"
                        val redColor = ContextCompat.getColor(this@MainActivity, R.color.red)
                        onOffLedButton?.setBackgroundColor(redColor)

                    }else if(responseBody.equals("OFF")){
                        isLedOn = false
                        onOffLedButton?.text = "Turn on LED"
                        val greenColor = ContextCompat.getColor(this@MainActivity, R.color.green)
                        onOffLedButton?.setBackgroundColor(greenColor)
                    }
                }
            }
            catch(e: Exception){
                textInfo?.text = "An error occurred 3"
            }
        }
    }

    fun checkStatusOfAlarm(){
        val url = "http://${deviceAddress}/alarmStatus"

        lifecycleScope.launch {
            try {
                // Check if device is avilable in the network
                if (!isDeviceReachable(deviceAddress)) {
                    textInfo?.text = "Device is not avilable in this network"
                    return@launch
                }
                val responseBody = sendGetRequest(url)
                if (responseBody != null) {
                    if (responseBody.equals("NO_ALARM")) {
                        textInfo?.text = "No alarm set"
                    } else {
                        val parts = responseBody.split(":")
                        textInfo?.text =
                            "Alarm is set on ${parts[1]}:${parts[2]}: ${parts[3]} \n"
                    }           // ALARM SET: HH:MM and preminutes: XX
                    Log.d("NetworkResponse", "Response: $responseBody")
                } else {
                    textInfo?.text = "Failed to get alarm status"
                    Log.e("NetworkResponse", "Request failed or no response body.")
                }

            } catch (e: IOException) {
                textInfo?.text = "Network error occurred"
                Log.e("NetworkRequest", "Network Error: ${e.message}", e)
            } catch(e: IndexOutOfBoundsException){
                textInfo?.text = "No alarm set"

            }catch (e: Exception) {
                textInfo?.text = "An error occurred 4"
                Log.e("NetworkRequest", "Error: ${e.message}", e)
            }
        }

    }

    private suspend fun isDeviceReachable(ipAddress: String): Boolean {
        return withContext(Dispatchers.IO) {
            try {
                val timeoutMs = 2000
                val inet = InetAddress.getByName(ipAddress)
                inet.isReachable(timeoutMs)
            } catch (e: IOException) {
                // Host not found or other network error
                false
            }
        }
    }
    fun onDeleteButtonClick() {
        val url = "http://192.168.0.150/stopalarm"

        lifecycleScope.launch {
            try {
                val responseBody = sendGetRequest(url)
                if (responseBody != null) {
                    if(responseBody.equals("Alarm stopped")){
                        textInfo?.text = "Alarm deleted"
                    }else if(responseBody.equals("No alarm is set"))
                    {
                        textInfo?.text = "No alarm set"
                    }
                    Log.d("NetworkResponse", "Response: $responseBody")
                } else {
                    textInfo?.text = "Failed to delete alarm"
                    Log.e("NetworkResponse", "Request failed or no response body.")
                }
            } catch (e: IOException) {
                textInfo?.text = "Network error occurred"
                Log.e("NetworkRequest", "Network Error: ${e.message}", e)
            } catch (e: Exception) {
                textInfo?.text = "An error occurred 5"
                Log.e("NetworkRequest", "Error: ${e.message}", e)
            }
        }
    }
    fun onSetButtonClick() {


        val hour = if (android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.M) {
            timePicker?.hour
        } else {
            timePicker?.currentHour // Deprecated in API 23
        }
        val minute = if (android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.M) {
            timePicker?.minute
        } else {
            timePicker?.currentMinute // Deprecated in API 23
        }

        val urlAlarm = "http://${deviceAddress}/setalarm?time=${prepareDigit(hour)}:${prepareDigit(minute)}&prealarm=${prepareDigit(preMinutePicker?.value)}"
        val urlSetLedOnMinutes = "http://${deviceAddress}/ledonminutes?minutes=${sharedPreferences.getInt("ledOnMinutes", 5)}"
        val urlSetBlinking = "http://${deviceAddress}/setflashing?repetitions=${sharedPreferences.getInt("blinkRepetitions", 0)}"

        Log.d("Url Alarm: ", urlAlarm)
        Log.d("Url Blinking: ", urlSetBlinking)
        Log.d("Url Repetitions: ", urlSetLedOnMinutes)

        lifecycleScope.launch {
            try {
                // Check if device is avilable in the network
                if(!isDeviceReachable(deviceAddress)) {
                    textInfo?.text = "Device is not avilable in this network"
                    return@launch
                }

                // Firstly set for how long LED should be on
                val responseBodyLedOnMinutes = sendGetRequest(urlSetLedOnMinutes)
                if (responseBodyLedOnMinutes != null) {
                    Log.d("NetworkResponse", "Response: $responseBodyLedOnMinutes")
                } else {
                    textInfo?.text = "Failed to set LED on minutes"
                    return@launch // exit because if this failes, the other ones will also fail
                    Log.e("NetworkResponse", "Request failed or no response body.")
                }

                // Then set how many times LED should blink
                val responseBodyBlinking = sendGetRequest(urlSetBlinking)
                if (responseBodyBlinking != null) {
                    Log.d("NetworkResponse", "Response: $responseBodyBlinking")
                } else {
                    textInfo?.text = "Failed to set blinking"
                    return@launch // exit because if this failes, the other ones will also fail
                    Log.e("NetworkResponse", "Request failed or no response body.")
                }

                // Finally set the alarm time and pre-alarm
                val responseBody = sendGetRequest(urlAlarm)
                if (responseBody != null) {
                    textInfo?.text = "Set to ${prepareDigit(hour)}:${prepareDigit(minute)} with Pre-alarm ${prepareDigit(preMinutePicker?.value)} minutes"
                    Log.d("NetworkResponse", "Response: $responseBody")
                } else {
                    textInfo?.text = "Failed to set alarm"
                    Log.e("NetworkResponse", "Request failed or no response body.")
                }

            } catch (e: IOException) {
                textInfo?.text = "Network error occurred"
                Log.e("NetworkRequest", "Network Error: ${e.message}", e)
            } catch (e: Exception) {
                textInfo?.text = "An error occurred 6"
                Log.e("NetworkRequest", "Error: ${e.message}", e)
            }
        }
    }

    private suspend fun sendGetRequest(urlString: String): String? {
        return withContext(Dispatchers.IO) { // Switch to IO Dispatcher for network operations
            val request = Request.Builder()
                .url(urlString)
                .build()

            try {
                val response = client.newCall(request).execute()
                if (response.isSuccessful) {
                    response.body?.string() // Returns the response body as a String
                } else {
                    Log.e("OkHttpRequest", "Unsuccessful response: ${response.code} ${response.message}")
                    null
                }
            } catch (e: IOException) {
                Log.e("OkHttpRequest", "IOException: ${e.message}", e)
                throw e
            }
        }
    }

    // This function is used to return one digits like 7 to 07 because controller can only take digits like that
    fun prepareDigit(digit: Int?): String {
     return if (digit!! < 10)"0$digit" else "$digit"
    }
}