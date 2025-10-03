package com.ledrise

import android.os.Bundle
import android.util.Log
import android.widget.Button
import android.widget.TimePicker
import androidx.activity.enableEdgeToEdge
import androidx.appcompat.app.AppCompatActivity
import androidx.core.view.ViewCompat
import androidx.core.view.WindowInsetsCompat
import android.widget.NumberPicker
import android.widget.TextView
import androidx.lifecycle.lifecycleScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext
import okhttp3.OkHttpClient
import okhttp3.Request
import java.io.IOException
import java.net.InetAddress
class MainActivity : AppCompatActivity() {

    var preMinutePicker: NumberPicker? = null
    var timePicker: TimePicker? = null
    var setButton: Button? = null
    var deleteButton: Button? = null
    private val client = OkHttpClient()
    val deviceIPAddress:String = "192.168.0.150"

    var textInfo: TextView? = null

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        enableEdgeToEdge()
        setContentView(R.layout.activity_main)
        ViewCompat.setOnApplyWindowInsetsListener(findViewById(R.id.main)) { v, insets ->
            val systemBars = insets.getInsets(WindowInsetsCompat.Type.systemBars())
            v.setPadding(systemBars.left, systemBars.top, systemBars.right, systemBars.bottom)
            insets
        }
        timePicker = findViewById<TimePicker>(R.id.timePicker)
        timePicker?.setIs24HourView(true)

        preMinutePicker = findViewById<NumberPicker>(R.id.minutePicker)
        // val selectedMinuteTextView = findViewById<TextView>(R.id.selectedMinuteTextView) // Optional: For display

        // Configure the NumberPicker for minutes (0-59)
        preMinutePicker?.minValue = 0
        preMinutePicker?.maxValue = 160
        preMinutePicker?.wrapSelectorWheel = true // Allows continuous scrolling (59 -> 0 and 0 -> 59)
        preMinutePicker?.value = 60 // Example: Set initial value to 30 minutes

        textInfo = findViewById<TextView>(R.id.textViewSetInfo)

        setButton = findViewById<Button>(R.id.buttonSet)
        setButton?.setOnClickListener {
            onSetButtonClick()
        }
        deleteButton = findViewById<Button>(R.id.buttonDeleteAlarm)
        deleteButton?.setOnClickListener{
            onDeleteButtonClick()

        }

        // After initialization check if alarm is set
        checkStatusOfAlarm();

    }

    fun checkStatusOfAlarm(){
        val url = "http://192.168.0.150/status"
        //val deviceIPAddress:String = "192.168.0.150"

        lifecycleScope.launch {
            try {
                // Check if device is avilable in the network
                if(!isDeviceReachable(deviceIPAddress)) {
                    textInfo?.text = "Device is not avilable in this network"
                    return@launch
                }
                val responseBody = sendGetRequest(url)
                if(responseBody != null) {
                    if (responseBody.equals("NO_ALARM")) {
                        textInfo?.text = "No alarm set"
                    } else {
                        val parts = responseBody.split(":")
                        textInfo?.text =
                            "Alarm is set on ${parts[1]}:${parts[2]} with ${parts[3]} minutes Pre-alarm \n"
                    }
                    Log.d("NetworkResponse", "Response: $responseBody")
                }else{
                    textInfo?.text = "Failed to get alarm status"
                    Log.e("NetworkResponse", "Request failed or no response body.")
                }

            } catch (e: IOException) {
                textInfo?.text = "Network error occurred"
                Log.e("NetworkRequest", "Network Error: ${e.message}", e)
            } catch (e: Exception) {
                textInfo?.text = "An error occurred"
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
                textInfo?.text = "An error occurred"
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

        val url = "http://192.168.0.150/setalarm?time=${prepareDigit(hour)}:${prepareDigit(minute)}&prealarm=${prepareDigit(preMinutePicker?.value)}"
        Log.d("URL: ", url)
        //val deviceIPAddress:String = "192.168.0.150"
        lifecycleScope.launch {
            try {
                // Check if device is avilable in the network
                if(!isDeviceReachable(deviceIPAddress)) {
                    textInfo?.text = "Device is not avilable in this network"
                    return@launch
                }
                val responseBody = sendGetRequest(url)
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
                textInfo?.text = "An error occurred"
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