package com.ledrise

import android.os.Bundle
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.Button
import android.widget.ImageButton
import android.widget.NumberPicker
import android.widget.TextView
import android.widget.TimePicker
import android.widget.Toast
import androidx.activity.enableEdgeToEdge
import androidx.appcompat.app.AppCompatActivity
import androidx.core.content.ContextCompat
import androidx.core.view.ViewCompat
import androidx.core.view.WindowInsetsCompat
import androidx.lifecycle.lifecycleScope
import androidx.recyclerview.widget.LinearLayoutManager
import androidx.recyclerview.widget.RecyclerView
import com.google.android.material.button.MaterialButton
import com.google.android.material.button.MaterialButtonToggleGroup
import com.google.android.material.dialog.MaterialAlertDialogBuilder
import com.google.android.material.materialswitch.MaterialSwitch
import com.google.android.material.slider.Slider
import kotlinx.coroutines.launch
import java.io.IOException

/** Weekday names indexed like the device bitmask: 0 = Sunday ... 6 = Saturday. */
private val DAY_NAMES = arrayOf("Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat")
/** Order shown in the UI (Monday first) as device weekday indexes. */
private val DAY_ORDER = intArrayOf(1, 2, 3, 4, 5, 6, 0)

class MainActivity : AppCompatActivity() {

    private lateinit var api: DeviceApi
    private lateinit var adapter: AlarmAdapter
    private lateinit var textInfo: TextView
    private lateinit var textEmpty: TextView
    private lateinit var onOffLedButton: Button
    private lateinit var brightnessSlider: Slider
    private var lastBrightnessSent = 0L

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        enableEdgeToEdge()
        setContentView(R.layout.activity_main)
        ViewCompat.setOnApplyWindowInsetsListener(findViewById(R.id.main)) { v, insets ->
            val systemBars = insets.getInsets(WindowInsetsCompat.Type.systemBars())
            v.setPadding(systemBars.left, systemBars.top, systemBars.right, systemBars.bottom)
            insets
        }
        api = DeviceApi.get(this)

        textInfo = findViewById(R.id.textViewSetInfo)
        textEmpty = findViewById(R.id.textEmpty)
        textInfo.setOnClickListener { refresh(forceScan = true) }

        adapter = AlarmAdapter(
            onClick = { showAlarmDialog(it) },
            onToggle = { alarm, enabled -> saveAlarm(alarm.copy(enabled = enabled)) }
        )
        findViewById<RecyclerView>(R.id.alarmList).apply {
            layoutManager = LinearLayoutManager(this@MainActivity)
            adapter = this@MainActivity.adapter
        }

        findViewById<Button>(R.id.buttonAddAlarm).setOnClickListener { showAlarmDialog(null) }
        findViewById<Button>(R.id.buttonStopAlarm).setOnClickListener { stopSunrise() }
        findViewById<ImageButton>(R.id.buttonSettings).setOnClickListener {
            startActivity(android.content.Intent(this, SettingsActivity::class.java))
        }

        onOffLedButton = findViewById(R.id.buttonTurnOnOffLed)
        onOffLedButton.setOnClickListener { toggleLed() }

        brightnessSlider = findViewById(R.id.brightnessSlider)
        brightnessSlider.addOnChangeListener { _, value, fromUser ->
            // throttle: the ESP32 handles requests one by one
            val now = System.currentTimeMillis()
            if (fromUser && now - lastBrightnessSent > 150) {
                lastBrightnessSent = now
                sendBrightness(value.toInt())
            }
        }
        brightnessSlider.addOnSliderTouchListener(object : Slider.OnSliderTouchListener {
            override fun onStartTrackingTouch(slider: Slider) {}
            override fun onStopTrackingTouch(slider: Slider) {
                sendBrightness(slider.value.toInt())
            }
        })
    }

    override fun onResume() {
        super.onResume()
        refresh(forceScan = false)
    }

    // ------------------------------------------------------------------ data

    private fun refresh(forceScan: Boolean) {
        textInfo.text = "Searching for device..."
        lifecycleScope.launch {
            try {
                if (forceScan) api.discover()
                val info = api.getInfo()
                val alarms = api.getAlarms()
                adapter.submit(alarms)
                textEmpty.visibility = if (alarms.isEmpty()) View.VISIBLE else View.GONE
                setLedButton(info.ledOn)
                textInfo.text = buildString {
                    append("Connected to ${api.host}")
                    if (!info.timeValid) append("\nWarning: device clock not synced yet, alarms are inactive")
                    if (info.sequenceRunning) append("\nSunrise is running")
                }
            } catch (e: IOException) {
                textInfo.text = "Device not found in this network. Tap to retry."
            } catch (e: Exception) {
                textInfo.text = "Error: ${e.message}"
            }
        }
    }

    private fun saveAlarm(alarm: Alarm, isNew: Boolean = false) {
        lifecycleScope.launch {
            try {
                api.saveAlarm(
                    if (isNew) null else alarm.id,
                    alarm.hour, alarm.minute, alarm.preMinutes, alarm.days, alarm.repeat, alarm.enabled
                )
            } catch (e: Exception) {
                showError(e)
            }
            refresh(forceScan = false)
        }
    }

    private fun deleteAlarm(alarm: Alarm) {
        lifecycleScope.launch {
            try {
                api.deleteAlarm(alarm.id)
            } catch (e: Exception) {
                showError(e)
            }
            refresh(forceScan = false)
        }
    }

    private fun stopSunrise() {
        lifecycleScope.launch {
            try {
                api.stopAlarm()
                Toast.makeText(this@MainActivity, "Sunrise stopped", Toast.LENGTH_SHORT).show()
            } catch (e: DeviceException) {
                Toast.makeText(this@MainActivity, e.message, Toast.LENGTH_SHORT).show()
            } catch (e: Exception) {
                showError(e)
            }
            refresh(forceScan = false)
        }
    }

    private fun toggleLed() {
        lifecycleScope.launch {
            try {
                setLedButton(api.toggleLed() == "LED turned on")
            } catch (e: Exception) {
                showError(e)
            }
        }
    }

    private fun sendBrightness(value: Int) {
        lifecycleScope.launch {
            try {
                api.setBrightness(value)
                setLedButton(value > 0)
            } catch (e: Exception) {
                showError(e)
            }
        }
    }

    private fun setLedButton(on: Boolean) {
        onOffLedButton.text = if (on) "Turn off LED" else "Turn on LED"
        val color = ContextCompat.getColor(this, if (on) R.color.red else R.color.green)
        onOffLedButton.setBackgroundColor(color)
    }

    private fun showError(e: Exception) {
        val msg = if (e is IOException) "Device not reachable" else e.message ?: "Error"
        Toast.makeText(this, msg, Toast.LENGTH_SHORT).show()
    }

    // ------------------------------------------------------------------ alarm dialog

    private fun showAlarmDialog(existing: Alarm?) {
        val view = LayoutInflater.from(this).inflate(R.layout.dialog_alarm_edit, null)
        val timePicker = view.findViewById<TimePicker>(R.id.dialogTimePicker)
        val preMinutes = view.findViewById<NumberPicker>(R.id.dialogPreMinutes)
        val daysGroup = view.findViewById<MaterialButtonToggleGroup>(R.id.dialogDays)
        val repeat = view.findViewById<MaterialSwitch>(R.id.dialogRepeat)

        timePicker.setIs24HourView(true)
        timePicker.hour = existing?.hour ?: 7
        timePicker.minute = existing?.minute ?: 0

        preMinutes.minValue = 1
        preMinutes.maxValue = 120
        preMinutes.value = existing?.preMinutes ?: 15

        val labels = arrayOf("Mo", "Tu", "We", "Th", "Fr", "Sa", "Su")
        val buttonForDay = HashMap<Int, MaterialButton>()
        DAY_ORDER.forEachIndexed { i, day ->
            val b = MaterialButton(this, null, com.google.android.material.R.attr.materialButtonOutlinedStyle)
            b.text = labels[i]
            b.isCheckable = true
            b.setPadding(0, 0, 0, 0)
            b.layoutParams = android.widget.LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.WRAP_CONTENT, 1f)
            b.id = View.generateViewId()
            daysGroup.addView(b)
            buttonForDay[day] = b
        }
        val initialDays = existing?.days ?: 0
        buttonForDay.forEach { (day, b) -> b.isChecked = initialDays and (1 shl day) != 0 }
        repeat.isChecked = existing?.repeat ?: true

        val builder = MaterialAlertDialogBuilder(this)
            .setTitle(if (existing == null) "New alarm" else "Edit alarm")
            .setView(view)
            .setNegativeButton("Cancel", null)
            .setPositiveButton("Save", null) // overridden below so the dialog stays on validation errors
        if (existing != null) {
            builder.setNeutralButton("Delete") { _, _ -> deleteAlarm(existing) }
        }
        val dialog = builder.create()
        dialog.setOnShowListener {
            dialog.getButton(android.app.Dialog.BUTTON_POSITIVE).setOnClickListener {
                var days = 0
                buttonForDay.forEach { (day, b) -> if (b.isChecked) days = days or (1 shl day) }
                if (days == 0) {
                    Toast.makeText(this, "Pick at least one day", Toast.LENGTH_SHORT).show()
                    return@setOnClickListener
                }
                timePicker.clearFocus()
                preMinutes.clearFocus()
                val alarm = Alarm(
                    id = existing?.id ?: -1,
                    hour = timePicker.hour,
                    minute = timePicker.minute,
                    preMinutes = preMinutes.value,
                    days = days,
                    repeat = repeat.isChecked,
                    enabled = true
                )
                saveAlarm(alarm, isNew = existing == null)
                dialog.dismiss()
            }
        }
        dialog.show()
    }
}

private fun describeDays(a: Alarm): String {
    val prefix = if (a.repeat) "Every " else "Once, "
    if (a.days == 127) return if (a.repeat) "Every day" else "Once, next day"
    return prefix + DAY_ORDER.filter { a.days and (1 shl it) != 0 }.joinToString(", ") { DAY_NAMES[it] }
}

private class AlarmAdapter(
    private val onClick: (Alarm) -> Unit,
    private val onToggle: (Alarm, Boolean) -> Unit
) : RecyclerView.Adapter<AlarmAdapter.Holder>() {

    private var items: List<Alarm> = emptyList()

    fun submit(list: List<Alarm>) {
        items = list.sortedWith(compareBy({ it.hour }, { it.minute }))
        notifyDataSetChanged()
    }

    class Holder(v: View) : RecyclerView.ViewHolder(v) {
        val time: TextView = v.findViewById(R.id.alarmTime)
        val days: TextView = v.findViewById(R.id.alarmDays)
        val pre: TextView = v.findViewById(R.id.alarmPre)
        val toggle: MaterialSwitch = v.findViewById(R.id.alarmSwitch)
    }

    override fun onCreateViewHolder(parent: ViewGroup, viewType: Int) =
        Holder(LayoutInflater.from(parent.context).inflate(R.layout.item_alarm, parent, false))

    override fun getItemCount() = items.size

    override fun onBindViewHolder(h: Holder, position: Int) {
        val a = items[position]
        h.time.text = "%02d:%02d".format(a.hour, a.minute)
        h.days.text = describeDays(a)
        h.pre.text = "Sunrise ${a.preMinutes} min"
        h.itemView.alpha = if (a.enabled) 1f else 0.5f
        h.itemView.setOnClickListener { onClick(a) }
        h.toggle.setOnCheckedChangeListener(null)
        h.toggle.isChecked = a.enabled
        h.toggle.setOnCheckedChangeListener { _, checked -> onToggle(a, checked) }
    }
}
