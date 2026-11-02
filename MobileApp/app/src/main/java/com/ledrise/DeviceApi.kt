package com.ledrise

import android.content.Context
import android.net.ConnectivityManager
import android.util.Log
import kotlinx.coroutines.CompletableDeferred
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.cancelChildren
import kotlinx.coroutines.coroutineScope
import kotlinx.coroutines.joinAll
import kotlinx.coroutines.launch
import kotlinx.coroutines.suspendCancellableCoroutine
import kotlinx.coroutines.sync.Semaphore
import kotlinx.coroutines.sync.withPermit
import okhttp3.Call
import okhttp3.Callback
import okhttp3.OkHttpClient
import okhttp3.Request
import okhttp3.Response
import org.json.JSONObject
import java.io.IOException
import java.net.Inet4Address
import java.util.concurrent.TimeUnit
import kotlin.coroutines.resume
import kotlin.coroutines.resumeWithException

/** HTTP error returned by the device (e.g. 400 with a message). Not a connectivity problem. */
class DeviceException(val code: Int, message: String) : Exception(message)

data class Alarm(
    val id: Int,
    val hour: Int,
    val minute: Int,
    val preMinutes: Int,
    val days: Int,       // bit 0 = Sunday ... bit 6 = Saturday
    val repeat: Boolean,
    val enabled: Boolean
)

data class DeviceInfo(
    val ip: String,
    val ledOn: Boolean,
    val sequenceRunning: Boolean,
    val timeValid: Boolean,
    val time: String,
    val ledOnMinutes: Int,
    val flashingRepetitions: Int
)

/**
 * Talks to the ESP32. The device address is not hardcoded: it is remembered after the first
 * successful contact and, when it stops answering, found again by scanning the local subnet
 * for a host that answers `GET /info` with {"device":"LedRise"}.
 */
class DeviceApi private constructor(context: Context) {

    private val appContext = context.applicationContext
    private val prefs = appContext.getSharedPreferences("AlarmSettings", Context.MODE_PRIVATE)

    private val client = OkHttpClient.Builder()
        .connectTimeout(3, TimeUnit.SECONDS)
        .readTimeout(5, TimeUnit.SECONDS)
        .build()

    private val scanClient = client.newBuilder()
        .connectTimeout(500, TimeUnit.MILLISECONDS)
        .readTimeout(800, TimeUnit.MILLISECONDS)
        .callTimeout(1200, TimeUnit.MILLISECONDS)
        .build()

    var host: String?
        get() = prefs.getString(KEY_HOST, null)
        private set(value) {
            prefs.edit().putString(KEY_HOST, value).apply()
        }

    // ------------------------------------------------------------------ high level API

    suspend fun getInfo(): DeviceInfo {
        val j = JSONObject(get("/info"))
        return DeviceInfo(
            ip = j.optString("ip"),
            ledOn = j.optBoolean("ledOn"),
            sequenceRunning = j.optBoolean("sequenceRunning"),
            timeValid = j.optBoolean("timeValid"),
            time = j.optString("time"),
            ledOnMinutes = j.optInt("ledOnMinutes", 5),
            flashingRepetitions = j.optInt("flashingRepetitions", 0)
        )
    }

    suspend fun getAlarms(): List<Alarm> {
        val arr = JSONObject(get("/alarms")).getJSONArray("alarms")
        return (0 until arr.length()).map { parseAlarm(arr.getJSONObject(it)) }
    }

    /** Creates (id == null) or updates an alarm. */
    suspend fun saveAlarm(
        id: Int?, hour: Int, minute: Int, preMinutes: Int,
        days: Int, repeat: Boolean, enabled: Boolean
    ): Alarm {
        val time = "%02d:%02d".format(hour, minute)
        var path = "/alarms/set?time=$time&prealarm=$preMinutes&days=$days" +
                "&repeat=${if (repeat) 1 else 0}&enabled=${if (enabled) 1 else 0}"
        if (id != null) path += "&id=$id"
        return parseAlarm(JSONObject(get(path)))
    }

    suspend fun deleteAlarm(id: Int) {
        get("/alarms/delete?id=$id")
    }

    suspend fun stopAlarm(): String = get("/stopalarm")

    suspend fun setLedOnMinutes(minutes: Int) {
        get("/ledonminutes?minutes=$minutes")
    }

    suspend fun setFlashing(repetitions: Int) {
        get("/setflashing?repetitions=$repetitions")
    }

    suspend fun toggleLed(): String = get("/turnOnOffLed")

    suspend fun setBrightness(value: Int) {
        get("/setBrightness?value=$value")
    }

    private fun parseAlarm(j: JSONObject) = Alarm(
        id = j.getInt("id"),
        hour = j.getInt("hour"),
        minute = j.getInt("minute"),
        preMinutes = j.getInt("pre"),
        days = j.getInt("days"),
        repeat = j.getBoolean("repeat"),
        enabled = j.getBoolean("enabled")
    )

    // ------------------------------------------------------------------ transport

    /**
     * GET on the device. Uses the remembered address first; if it does not answer,
     * scans the network once and retries. Throws IOException when the device can't be found.
     */
    suspend fun get(path: String): String {
        host?.let { saved ->
            try {
                return request(client, saved, path)
            } catch (e: IOException) {
                Log.w(TAG, "Saved host $saved not responding: ${e.message}")
            }
        }
        val found = discover() ?: throw IOException("Device not found in this network")
        return request(client, found, path)
    }

    private suspend fun request(c: OkHttpClient, host: String, path: String): String {
        val req = Request.Builder().url("http://$host$path").build()
        c.newCall(req).await().use { resp ->
            val body = resp.body?.string().orEmpty()
            if (!resp.isSuccessful) throw DeviceException(resp.code, body.ifEmpty { resp.message })
            return body
        }
    }

    // ------------------------------------------------------------------ discovery

    /** Scans the local /24 subnet; stores and returns the address of the first LedRise found. */
    suspend fun discover(): String? {
        val candidates = subnetHosts()
        Log.d(TAG, "Scanning ${candidates.size} addresses")
        val found = scan(candidates)
        if (found != null) host = found
        return found
    }

    private suspend fun scan(candidates: List<String>): String? = coroutineScope {
        val result = CompletableDeferred<String?>()
        val limiter = Semaphore(48)
        val jobs = candidates.map { candidate ->
            launch(Dispatchers.IO) {
                limiter.withPermit {
                    if (!result.isCompleted && isLedRise(candidate)) result.complete(candidate)
                }
            }
        }
        launch { jobs.joinAll(); result.complete(null) }
        val winner = result.await()
        coroutineContext.cancelChildren()
        winner
    }

    private suspend fun isLedRise(candidate: String): Boolean = try {
        val json = JSONObject(request(scanClient, candidate, "/info"))
        json.optString("device") == "LedRise"
    } catch (e: Exception) {
        false
    }

    /** All addresses of the Wi-Fi /24 network except our own. */
    private fun subnetHosts(): List<String> {
        val cm = appContext.getSystemService(Context.CONNECTIVITY_SERVICE) as ConnectivityManager
        val props = cm.getLinkProperties(cm.activeNetwork) ?: return emptyList()
        val addr = props.linkAddresses.firstOrNull { it.address is Inet4Address } ?: return emptyList()
        val bytes = addr.address.address
        val own = bytes[3].toInt() and 0xFF
        val prefix = "${bytes[0].toInt() and 0xFF}.${bytes[1].toInt() and 0xFF}.${bytes[2].toInt() and 0xFF}"
        return (1..254).filter { it != own }.map { "$prefix.$it" }
    }

    companion object {
        private const val TAG = "DeviceApi"
        private const val KEY_HOST = "deviceHost"

        @Volatile
        private var instance: DeviceApi? = null

        fun get(context: Context): DeviceApi =
            instance ?: synchronized(this) {
                instance ?: DeviceApi(context).also { instance = it }
            }
    }
}

private suspend fun Call.await(): Response = suspendCancellableCoroutine { cont ->
    cont.invokeOnCancellation { cancel() }
    enqueue(object : Callback {
        override fun onFailure(call: Call, e: IOException) {
            if (!cont.isCancelled) cont.resumeWithException(e)
        }

        override fun onResponse(call: Call, response: Response) {
            cont.resume(response)
        }
    })
}
